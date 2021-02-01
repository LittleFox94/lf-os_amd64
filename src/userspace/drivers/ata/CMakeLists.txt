project(ata C)

add_executable(ata main.c ata_generic.c)
install(TARGETS ata DESTINATION LFOS)


