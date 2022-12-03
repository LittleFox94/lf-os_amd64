#include <efi.h>
#include <protocol/efi-pbcp.h>
#include <protocol/efi-sfsp.h>
#include <protocol/efi-fp.h>

#include <string.h>

static EFI_GUID gEfiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
static EFI_GUID gEfiPxeBaseCodeProtocolGuid      = EFI_PXE_BASE_CODE_PROTOCOL_GUID;

typedef struct {
    EFI_PXE_BASE_CODE_PROTOCOL* pxe;
    EFI_IP_ADDRESS              tftpAddress;
    BOOLEAN                     ipv6;

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL sfsp;
} PXE_SFSP_PRIVATE_DATA;

typedef struct {
    EFI_FILE_PROTOCOL fp;

    PXE_SFSP_PRIVATE_DATA* pxeSfsp;

    UINTN   bufferSize;
    VOID*   buffer;
    char    path[0];
} PXE_FP_PRIVATE_DATA;

static void fill_pxefp_interface(EFI_FILE_PROTOCOL* fp);

#define CR(Record, Type, Field) (Type*)(((VOID*)Record) - (VOID*)(&(((Type*)0)->Field)))
#define SFS_PRIVATE(this) PXE_SFSP_PRIVATE_DATA* private = CR(this, PXE_SFSP_PRIVATE_DATA, sfsp);
#define FP_PRIVATE(this)  PXE_FP_PRIVATE_DATA*   private = CR(this, PXE_FP_PRIVATE_DATA, fp);

/**
 * @param filename Name of the file
 * @param fp       in: parent pxefp private, out: new pxefp private
 * @returns        EFI_STATUS
 */
static EFI_STATUS mk_pxefp_private(CHAR16* filename, PXE_FP_PRIVATE_DATA** fp) {
    EFI_STATUS status;
    PXE_FP_PRIVATE_DATA* parent = *fp;

    UINTN len = wcslen(filename) + 1;

    if(parent) {
        len += strlen(parent->path);
    }

    if((status = BS->AllocatePool(EfiBootServicesData, sizeof(PXE_FP_PRIVATE_DATA) + len, (VOID**)fp)) & EFI_ERR) {
        return status;
    }

    memset(*fp, 0, sizeof(PXE_FP_PRIVATE_DATA) + len);

    if(parent) {
        strcpy((*fp)->path, parent->path);
        wcstombs((*fp)->path + strlen(parent->path), filename, wcslen(filename));

        (*fp)->pxeSfsp = parent->pxeSfsp;
    }
    else {
        wcstombs((*fp)->path, filename, wcslen(filename));
    }

    fill_pxefp_interface(&(*fp)->fp);

    return EFI_SUCCESS;
}

EFI_STATUS PxeFpOpen(EFI_FILE_PROTOCOL* this, EFI_FILE_PROTOCOL** new, CHAR16* filename, UINT64 openMode, UINT64 attributes) {
    FP_PRIVATE(this);
    EFI_STATUS status;

    PXE_FP_PRIVATE_DATA* fp = private;
    if((status = mk_pxefp_private(filename, &fp)) & EFI_ERR) {
        return status;
    }

    status = private->pxeSfsp->pxe->Mtftp(
                private->pxeSfsp->pxe,
                EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE,
                0,
                EFI_FALSE,
                &fp->bufferSize,
                0,
                &private->pxeSfsp->tftpAddress,
                (CHAR8*)fp->path,
                0,
                EFI_TRUE
            );

    if(status & EFI_ERR) {
        wprintf(L" no get file size ");

        status = private->pxeSfsp->pxe->Mtftp(
                    private->pxeSfsp->pxe,
                    EFI_PXE_BASE_CODE_TFTP_READ_FILE,
                    0,
                    EFI_FALSE,
                    &fp->bufferSize,
                    0,
                    &private->pxeSfsp->tftpAddress,
                    (CHAR8*)fp->path,
                    0,
                    EFI_FALSE
                );
    }

    if(status & EFI_ERR) {
        wprintf(L" no read file either ");
        BS->FreePool(fp);
        return status;
    }

    do {
        if(fp->buffer) {
            BS->FreePool(fp->buffer);
        }

        status = BS->AllocatePool(EfiBootServicesData, fp->bufferSize, &fp->buffer);
        if(status & EFI_ERR) {
            BS->FreePool(fp);
            return status;
        }

        static UINTN blockSize = 1468;

        status = private->pxeSfsp->pxe->Mtftp(
                    private->pxeSfsp->pxe,
                    EFI_PXE_BASE_CODE_TFTP_READ_FILE,
                    fp->buffer,
                    EFI_FALSE,
                    &fp->bufferSize,
                    &blockSize,
                    &private->pxeSfsp->tftpAddress,
                    (CHAR8*)fp->path,
                    0,
                    EFI_FALSE
            );

        if(status == EFI_BUFFER_TOO_SMALL) {
            fp->bufferSize += 16384;
        }
    } while(status == EFI_BUFFER_TOO_SMALL);

    if(status & EFI_ERR) {
        wprintf(L" no read file ");

        BS->FreePool(fp->buffer);
        BS->FreePool(fp);
        return status;
    }

    *new = &fp->fp;

    return EFI_SUCCESS;
}

EFI_STATUS PxeFpClose(EFI_FILE_PROTOCOL* this) {
    FP_PRIVATE(this);

    if(private->buffer) {
        private->bufferSize = 0;
        BS->FreePool(private->buffer);
    }

    return EFI_SUCCESS;
}

EFI_STATUS PxeFpDelete(EFI_FILE_PROTOCOL* this) {
    return EFI_UNSUPPORTED;
}

EFI_STATUS PxeFpRead(EFI_FILE_PROTOCOL* this, UINTN* bufferSize, VOID* buffer) {
    FP_PRIVATE(this);

    if(!bufferSize) {
        return EFI_INVALID_PARAMETER;
    }

    if(*bufferSize < private->bufferSize) {
        *bufferSize = private->bufferSize;
        return EFI_BUFFER_TOO_SMALL;
    }

    *bufferSize = private->bufferSize;

    if(!buffer) {
        return EFI_INVALID_PARAMETER;
    }

    BS->CopyMem(buffer, private->buffer, private->bufferSize);

    return EFI_SUCCESS;
}

EFI_STATUS PxeFpWrite(EFI_FILE_PROTOCOL* this, UINTN* bufferSize, VOID* buffer) {
    return EFI_UNSUPPORTED;
}

EFI_STATUS PxeFpGetPosition(EFI_FILE_PROTOCOL* this, UINT64* pos) {
    return EFI_UNSUPPORTED;
}

EFI_STATUS PxeFpSetPosition(EFI_FILE_PROTOCOL* this, UINT64 pos) {
    return EFI_UNSUPPORTED;
}

EFI_STATUS PxeFpGetInfo(EFI_FILE_PROTOCOL* this, EFI_GUID* infoType, UINTN* bufferSize, VOID* buffer) {
    return EFI_UNSUPPORTED;
}

EFI_STATUS PxeFpSetInfo(EFI_FILE_PROTOCOL* this, EFI_GUID* infoType, UINTN bufferSize, VOID* buffer) {
    return EFI_UNSUPPORTED;
}

EFI_STATUS PxeFpFlush(EFI_FILE_PROTOCOL* this) {
    return EFI_UNSUPPORTED;
}

EFI_STATUS PxeOpenVolume(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* this, EFI_FILE_PROTOCOL** root) {
    SFS_PRIVATE(this);
    EFI_STATUS status;

    PXE_FP_PRIVATE_DATA* fp = 0;
    if((status = mk_pxefp_private(L"/", &fp)) & EFI_ERR) {
        return status;
    }

    fp->pxeSfsp = private;

    *root = &fp->fp;
    return EFI_SUCCESS;
}

void fill_pxefp_interface(EFI_FILE_PROTOCOL* fp) {
    fp->Revision    = EFI_FILE_PROTOCOL_REVISION,
    fp->Open        = PxeFpOpen;
    fp->Close       = PxeFpClose;
    fp->Delete      = PxeFpDelete;
    fp->Read        = PxeFpRead;
    fp->Write       = PxeFpWrite;
    fp->GetPosition = PxeFpGetPosition;
    fp->SetPosition = PxeFpSetPosition;
    fp->GetInfo     = PxeFpGetInfo;
    fp->SetInfo     = PxeFpSetInfo;
    fp->Flush       = PxeFpFlush;

}

EFI_STATUS initPxeSfs(EFI_HANDLE* deviceHandle) {
    EFI_STATUS status;

    EFI_PXE_BASE_CODE_PROTOCOL* pxe;
    if((status = BS->HandleProtocol(*deviceHandle, &gEfiPxeBaseCodeProtocolGuid, (void**)&pxe)) & EFI_ERR) {
        return status;
    }
    PXE_SFSP_PRIVATE_DATA* private;
    if((status = BS->AllocatePool(EfiBootServicesData, sizeof(PXE_SFSP_PRIVATE_DATA), (VOID**)&private)) & EFI_ERR) {
        return status;
    }
    private->sfsp = (EFI_SIMPLE_FILE_SYSTEM_PROTOCOL){
        .Revision = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION,
        .OpenVolume = PxeOpenVolume,
    };

    private->pxe  = pxe;
    private->ipv6 = pxe->Mode->UsingIPv6;

    memset(&private->tftpAddress, 0, sizeof(EFI_IP_ADDRESS));

    if(private->ipv6) {
        wprintf(L"pxe: only IPv4 is supported for now\n");
        return EFI_UNSUPPORTED; // we have to parse the DHCP options then
    }
    else {
        EFI_PXE_BASE_CODE_DHCPV4_PACKET* packet = 0;

        if(pxe->Mode->DhcpAckReceived) {
            packet = &pxe->Mode->DhcpAck.Dhcpv4;
        }
        else if(pxe->Mode->PxeReplyReceived) {
            packet = &pxe->Mode->PxeReply.Dhcpv4;
        }
        else if(pxe->Mode->PxeBisReplyReceived) {
            packet = &pxe->Mode->PxeBisReply.Dhcpv4;
        }

        if(packet) {
            for(int i = 0; i < 4; ++i) {
                private->tftpAddress.v4.Addr[i] = packet->BootpSiAddr[i];
            }
        }

        if(private->tftpAddress.Addr[0] == 0) {
            wprintf(L"pxe: only bootp SiAddr is supported for now\n");
            return EFI_UNSUPPORTED;
        }

        wprintf(L"Booting via PXE from %d.%d.%d.%d (I'm %d.%d.%d.%d)\n",
            (int)private->tftpAddress.v4.Addr[0],
            (int)private->tftpAddress.v4.Addr[1],
            (int)private->tftpAddress.v4.Addr[2],
            (int)private->tftpAddress.v4.Addr[3],
            (int)private->pxe->Mode->StationIp.v4.Addr[0],
            (int)private->pxe->Mode->StationIp.v4.Addr[1],
            (int)private->pxe->Mode->StationIp.v4.Addr[2],
            (int)private->pxe->Mode->StationIp.v4.Addr[3]
        );
    }

    if((status = BS->InstallMultipleProtocolInterfaces(
            deviceHandle,
            &gEfiSimpleFileSystemProtocolGuid, &private->sfsp,
            0)) & EFI_ERR) {
        return status;
    }

    return EFI_SUCCESS;
}
