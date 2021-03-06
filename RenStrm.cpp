// ---------------------------------------------------------------------------
// NTFS Alternate Data Stream (ADS)  Rename Utility
//
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non - commercial, and by any
// 
// Written 2018-6-11 by Hernán Di Pietro.
// ---------------------------------------------------------------------------

#include "RenStrm.h"

int wmain(int argc, wchar_t** argv)
{
    if (argc != 3)
    {
        printf("Rename NTFS alternate data streams in a file.\n\n");
        printf("RENSTRM oldName newName\n\n");
        printf("oldName         The old stream name, including parent file name.\n");
        printf("newName         The new stream name, in :name:[$DATA] format.\n");
        printf("\nExamples:\n\n");
        printf("RENSTRM test.txt:A :U\n");
        printf("RENSTRM test.txt:A :X:$DATA\n");
        printf("RENSTRM e:\\temp\\test.txt:ABC :stream2");
        printf("\n");
        return 0;
    }

    const WCHAR* filename = argv[1];
    const WCHAR* newFilename = argv[2];
    
    HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
    if (!hNtDll) 
    {
        printf("Cannot get NTDLL.DLL module\n");
        return GetLastError();
    }

    NTSETINFORMATIONFILE NtSetInformationFile = (NTSETINFORMATIONFILE)GetProcAddress(hNtDll, "NtSetInformationFile");
    if (NtSetInformationFile == nullptr)
    {
        printf("Cannot get NtSetInformationFile routine address\n");
        return GetLastError();
    }

    RTLNTSTATUSTODOSERROR RtlNtStatusToDosError = (RTLNTSTATUSTODOSERROR)GetProcAddress(hNtDll, "RtlNtStatusToDosError");
    if (RtlNtStatusToDosError == nullptr)
    {
        printf("Cannot get RtlNtStatusToDosError routine address\n");
        return GetLastError();
    }

    UniqueHANDLE hFile(CreateFile(filename, DELETE | SYNCHRONIZE | GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));

    if (hFile == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Cannot open file %s (Win32 code = %d).\n", filename, GetLastError());
        return GetLastError();
    }

    const size_t BUFFERLEN = sizeof(FILE_RENAME_INFORMATION) + (sizeof(wchar_t) * (lstrlen(newFilename)));
    auto pFileRenameInfoBuf = std::make_unique<BYTE[]>(BUFFERLEN);
    
    PFILE_RENAME_INFORMATION pRenameInfo = (PFILE_RENAME_INFORMATION) pFileRenameInfoBuf.get();
    pRenameInfo->ReplaceIfExists = TRUE;
    pRenameInfo->RootDirectory = NULL;
    pRenameInfo->FileNameLength = sizeof(wchar_t) * (lstrlen(newFilename));

    memcpy(pRenameInfo->FileName, newFilename, sizeof(wchar_t) * lstrlen(newFilename));
    
    IO_STATUS_BLOCK iosb;
    NTSTATUS status = NtSetInformationFile(hFile, &iosb, pRenameInfo, BUFFERLEN, FileRenameInformation);
    if (NT_SUCCESS(status))
    {
        wprintf(L"Stream rename operation (%s -> %s) SUCCESS\n", filename, newFilename);
    }
    else
    {
        wprintf(L"Stream rename operation (%s -> %s) failed. Status code = 0x%08x\n", filename, newFilename, status);
        return RtlNtStatusToDosError(status);
    }

    return 0;
}

