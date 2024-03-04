/* Implement reed-solomon error correcting codes
 * and related mathematics.
 *
 */

#include <panic.h>

#include <log.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Since our whole number system only goes from 0 to 255, it's not too big of a
 * sacrifice to have precomputed logarithm and exponent tables for all things
 * multiplication&divison. This makes implementing most of finite field arithmetics
 * so So much more convenient.
 */
static uint8_t ff_exp_table[256];
static uint8_t ff_log_table[256];
static bool ff_table_init_done = false;

/* Pre-compute the exponent and logarithm tables */
static void init_ff_lookup_tables(void) {
    uint16_t b;
    uint8_t a;

    if (ff_table_init_done) {
        return;
    }
    ff_table_init_done = true;

    for (a = 0, b = 1; a < 255; a++) {
        ff_log_table[b] = a;
        ff_exp_table[a] = (uint8_t)b;

        // Multiply b by 2
        b = b << 1;
        if (b & 0x100) {
            // if b goes above our finite field bounds (above 256),
            // XOR it with our first irreducible/prime polynomial above 256, which is
            // 285. This should clear bits above 0xFF and set remainder for us..
            //
            b ^= 285;
        }
    }
}

/* Our finite field is from 0 to 255 (unsigned char), it's base
 * number is 2, so our field is GF(2^8). For all groups GF(2^n)
 * addition and subtraction are just essentially XOR, since as
 *
 *    a + a = 0
 *    a + b = b + a
 * our only numbers available are base 2, so we're left with
 *
 *    1 + 1 = 0
 *    1 + 0 = 1
 *    0 + 1 = 1
 *    1 + 1 = 0
 *
 * and same for subtraction, since a - b = a + (-b), and
 * as our number field is from 0 to 2^8, we don't have negative
 * numbers, leading to exactly same truth table as written above.
 *
 */
static uint8_t ff_add(uint8_t a, uint8_t b) {
    return a ^ b;
}

/* Multiplication is a bit different but nothing too spicy either;
 * we'll map our number as a vector space over GF(2);
 *    0a = 0
 *    1a = a
 * for all a, this just means we've our multiplier be in GF(2) and
 * the times it's multiplied with in GF(2^n).
 *
 * Another thing to note is that all our multiplications happen
 * mod irreducible reducing polynomial, so in practise, if I understood
 * this correctly, would be calculated A times B, and result xor'ed with
 * first irreducible polynomial that's above 2^8, in this case that'd be
 * 285, so any multiplication should be something like:
 *
 * uint8_t mul(uint8_t a, uint8_t b) {
 *     uint16_t ret = (a * b);
 *     if (ret > 256)
 *         ret = ret ^ 285;
 *     return ret;
 * }
 *
 * We can also rely on the fact that on our finite field, every number can be
 * represented as 2^n, this means that:
 *
 *    a * b = 2^n * 2^m
 *
 * and as log(a * b) = log(a) + log(b), we get the following:
 *
 *    a * b = 2^n * 2^m -> log(a * b) = log(n) + log(m)
 *
 * Note, that I'm using log to indicate logarithm base 2.
 * Anyway, as we now have our log(ab) = (log(n) + log(m)), all we need is
 * to get 'antilog'/exponent for resulting log(n) + log(m). Since our number field
 * is Very small, we can precompute lookup tables for logarithms and exponents, and
 * change our multiplication a * b to form: antilog(log(n) + log(m))
 *
 * If we just set up our lookup tables sensibly, we can also do this in a way
 * where a and b can be used as offsets to our tables, reducing whole
 * multiplication process into single addition and few memory accesses
 *
 */
static uint8_t ff_mul(uint8_t a, uint8_t b) {
    uint16_t off;

    if (!a || !b) {
        return 0;
    }

    off = ff_log_table[a] + ff_log_table[b];
    off = off % 255;
    return ff_exp_table[off];
}

/* Division is opposite of above, not really a lot that I feel like I can
 * reasonably explain here
 */
//static uint8_t ff_div(uint8_t a, uint8_t b) {
//    if (!b) {
//        panic_message("qr: divide by zero");
//    }
//    if (!a) return 0;
//
//    uint16_t off = (ff_log_table[a] + 255) - ff_log_table[b];
//    off = off % 255;
//    return ff_exp_table[off];
//}
//
/* Exponents are just table lookups too, since
 * we did precompute all of this already.
 *
 * Inverse is just 1 / base, so nothing too unusual there either
 */
static uint8_t ff_exp(uint8_t a, uint8_t b) {
    uint16_t off;

    /* Anything to power of 0 is 1 */
    if (!b) {
        return 1;
    }

    /* Anything to power of 1 is the number itself */
    if (b == 1) {
        return a;
    }

    off = ff_log_table[a] * b;
    off = off % 255;
    return ff_exp_table[off];
}

/* Convert a scalar number into a polynomial, so
 * essentially eg: 1011 0011 is to be represented as
 * x^7 + 0 + x^5 + x^4 + 0 + 0 + x + 1
 *
 * we can't reasonably deal with unknown variables in C, so
 * instead we'll only take coefficients as a list; so
 *  [1, 0, 1, 1, 0, 0, 1, 1]
 *
 * Now, since we're doing mathematics on the polynomials, we might
 * and will end up to situations where our polynomial is eg.
 * 8x^7 + 4x^5
 * which will then just be
 *  [8, 0, 4, 0, 0, 0, 0, 0]
 *
 * Once we start multiplying polynomials with eachother, our highest
 * term changes, we'll have to reallocate our polynomial coefficient
 * pointers as that happens, resulting to lists such as
 *  [8, 0, 4, 0, 0, 0, 0, 0, 0, 5]
 *
 * So, we'll have variable amount of coefficients, we'll need a counter
 * to keep track of how many coefficients we have per list.
 *
 */
typedef struct {
    uint8_t coefficients[256];
    int size;
} polynomial;

static void poly_init(polynomial *p, int size) {
    memset(p, 0, sizeof(polynomial));
    p->size = size;
}

/* Resize polynomial, in case we're allocating more space
 * for our polynomial, new memory is initialized to 0's
 */
//static void poly_resize(polynomial *p, int new_size) {
//    if (new_size < p->size) {
//        p->size = new_size;
//        return;
//    }
//    memset((p->coefficients+p->size), 0, (new_size - p->size));
//    p->size = new_size;
//}

/* Convert a scalar value into a polynomial */
void scalar_to_polynomial(polynomial *ret, uint8_t a) {
    poly_init(ret, 8);
    for (int i = 7; i >= 0; i--) {
        ret->coefficients[7 - i] = ((1 << i) & a);
    }
}

/* Convert a polynomial into a scalar value. Note, that
 * even if we have polynomials that have coefficients above
 * 2^8, our scalar values must remain within our finite field, so
 * we'll return uint8_t.
 *
 * We'll take uint8_t x as the value for evaluating 'x' in our
 * polynomial.
 */
// uint8_t polynomial_to_scalar(polynomial *p, uint8_t x) {
//     uint8_t ret;
//     int i;
//
//     for (ret = p->coefficients[0], i = 1; i < p->size; i++) {
//         ret = ff_add(p->coefficients[i], ff_mul(ret, x));
//     }
//     return ret;
// }

/* Multiply polynomial by another one,
 * whenever you multiply two polynomials, the highest coefficient will be
 * highest of poly a + highest of poly b - 1, so eg
 *  [4, 2, 5] * [5, 1, 3]
 *      = 4x^2 + 2x + 5 * 5x^2 + x + 3
 *      = (4x^2 * 5x^2 + 4x^2 * x + 4x^2 * 3) + (2x * 5x^2 + 2x * x + 2x * 3) + (5 * 5x^2 + 5 * x + 5 * 3)
 *      = (20x^4 + 4x^3 + 12x^2) + (10x^3 + 4x^2 + 6x) + (25x^2 + 5x + 15)
 *      = 20x^4 + 14x^3 + 37x^2 + 11x + 15
 */
static void poly_mul_by_poly(polynomial *a, polynomial *b) {
    polynomial result;

    poly_init(&result, (a->size + b->size - 1));
    for (int i = 0; i < a->size; i++) {
        for (int j = 0; j < b->size; j++) {
            result.coefficients[i + j] = ff_add(result.coefficients[i + j], ff_mul(a->coefficients[i], b->coefficients[j]));
        }
    }
    memcpy(a->coefficients, &result.coefficients, result.size);
    a->size = result.size;
}

//void dump_polynomial(polynomial *p) {
//    char buffer[512];
//    memset(buffer, 0x20, sizeof(buffer));
//
//    for(size_t i = 0; i < p->size; ++i) {
//        ksnprintf(buffer + (i*4), 4, "%d", p->coefficients[i]);
//    }
//    buffer[511] = 0;
//
//    logd("qr/rs", "polynomial: %s", buffer);
//}

void dump_uint8_t_p(uint8_t *p, size_t s) {
    char buffer[512];
    memset(buffer, 0x20, sizeof(buffer));

    for(size_t i = 0; i < s; ++i) {
        ksnprintf(buffer + (i*4), 4, "%3d", p[i]);
        buffer[(i * 4) - 1] = ' ';
    }
    buffer[511] = 0;

    logd("qr/rs", "uint8_t*: %s", buffer);
}

/* Divide polynomial by another using https://en.wikipedia.org/wiki/Synthetic_division#Expanded_synthetic_division
 * I tried to explain the expanded synthetic division here but wikipedia does way better job
 * at it than I do. Really don't feel like making ascii charts and stuff ^^"
 *
 * Initially I thought I'd name this function as polynomial division, but then I realised that since reed-solomon
 * error correcting codes only care about the remainder, I can just do all of that here and call this
 * the rs_encode function :3
 *
 */
static void rs_encode(polynomial *a, polynomial *b, uint8_t *out) {
    polynomial msg_out;
    poly_init(&msg_out, a->size);
    memcpy(msg_out.coefficients, a->coefficients, a->size);

    for (int i = 0; i <= ((a->size - b->size)); i++) {
        uint8_t c = msg_out.coefficients[i];
        if (c) {
            for (int j = 1; j < b->size; j++) {
                if (b->coefficients[j]) {
                    msg_out.coefficients[i + j] ^= ff_mul(b->coefficients[j], c);
                }
            }
        }
    }

    for (int i = (a->size - b->size + 1), j = 0; i <= a->size; i++, j++) {
        out[j] = msg_out.coefficients[i];
    }
}

/* Reed-solomon generator polynomial is essentially computed as:
 * g(b): for all (j = 1; j < (n - k); j++) {
 *  result *= (b - x^j)
 * }
 * where x is primitive element, n is number of error correcting symbols, and  k is
 * size of the message
 */
static void generator(polynomial *g, int n) {
    int i;
    polynomial mul;

    scalar_to_polynomial(g, 1);
    poly_init(&mul, 2);
    mul.coefficients[0] = 1;

    for (i = 0; i < n; i++) {
        mul.coefficients[1] = ff_exp(2, i);
        poly_mul_by_poly(g, &mul);
    }

    // Clean up unnecessary 0's
    for (i = 0; i < g->size; i++) {
        if (g->coefficients[i] != 0) {
            break;
        }
    }
    memmove(g->coefficients, g->coefficients+i, g->size - i);
    g->size -= i;
}

/* Our message is a polynomial consisting of sequence of coefficients.
 *
 */
void qr_block_ec_generate(uint8_t *block, size_t block_size, uint8_t *out, size_t nsym) {
    init_ff_lookup_tables();
    polynomial gen;
    polynomial msg;

    generator(&gen, nsym);

    // Simple bounds check
    if ((block_size + gen.size) >= 256) {
        panic_message("qr: invalid block size");
    }

    // Initialize message polynomial
    poly_init(&msg, (block_size + gen.size - 1));
    memcpy(&msg.coefficients, block, block_size);

    // Encode message
    rs_encode(&msg, &gen, out);
}


