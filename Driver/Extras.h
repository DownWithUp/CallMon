#pragma once

typedef struct _CUSTOM_HEADER
{
    ULONG64 ProcessId;
    ULONG64 StackData[0x10];
} CUSTOM_HEADER, * PCUSTOM_HEADER;

typedef struct _TOTAL_PACKET
{
    CUSTOM_HEADER CustomHeader;
    KTRAP_FRAME Frame;
} TOTAL_PACKET, * PTOTAL_PACKET;
