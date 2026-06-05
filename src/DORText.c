#include "DOR.h"
#include "DORInternal.h"

#include <stddef.h>

#define DORTextUnknownCharacter '?'

static const uint8_t DORTextEncodeTable[128] = {
    [' '] = 0x1E,

    ['A'] = 0x56, ['B'] = 0x57, ['C'] = 0x58, ['D'] = 0x59,
    ['E'] = 0x5A, ['F'] = 0x5B, ['G'] = 0x5C, ['H'] = 0x5D,
    ['I'] = 0x5E, ['J'] = 0x5F, ['K'] = 0x60, ['L'] = 0x61,
    ['M'] = 0x62, ['N'] = 0x63, ['O'] = 0x64, ['P'] = 0x65,
    ['Q'] = 0x66, ['R'] = 0x67, ['S'] = 0x68, ['T'] = 0x69,
    ['U'] = 0x6A, ['V'] = 0x6B, ['W'] = 0x6C, ['X'] = 0x6D,
    ['Y'] = 0x6E, ['Z'] = 0x6F,

    ['a'] = 0x73, ['b'] = 0x74, ['c'] = 0x75, ['d'] = 0x76,
    ['e'] = 0x77, ['f'] = 0x78, ['g'] = 0x79, ['h'] = 0x7A,
    ['i'] = 0x7B, ['j'] = 0x7C, ['k'] = 0x7D, ['l'] = 0x7E,
    ['m'] = 0x7F, ['n'] = 0x80, ['o'] = 0x81, ['p'] = 0x82,
    ['q'] = 0x83, ['r'] = 0x84, ['s'] = 0x85, ['t'] = 0x86,
    ['u'] = 0x87, ['v'] = 0x88, ['w'] = 0x89, ['x'] = 0x8A,
    ['y'] = 0x8B, ['z'] = 0x8C,

    ['1'] = 0x8F, ['2'] = 0x90, ['3'] = 0x91, ['4'] = 0x92,
    ['5'] = 0x93, ['6'] = 0x94, ['7'] = 0x95, ['8'] = 0x96,
    ['9'] = 0x97, ['0'] = 0x98,
};

static const char DORTextDecodeTable[256] = {
    [0x1E] = ' ',

    [0x56] = 'A', [0x57] = 'B', [0x58] = 'C', [0x59] = 'D',
    [0x5A] = 'E', [0x5B] = 'F', [0x5C] = 'G', [0x5D] = 'H',
    [0x5E] = 'I', [0x5F] = 'J', [0x60] = 'K', [0x61] = 'L',
    [0x62] = 'M', [0x63] = 'N', [0x64] = 'O', [0x65] = 'P',
    [0x66] = 'Q', [0x67] = 'R', [0x68] = 'S', [0x69] = 'T',
    [0x6A] = 'U', [0x6B] = 'V', [0x6C] = 'W', [0x6D] = 'X',
    [0x6E] = 'Y', [0x6F] = 'Z',

    [0x73] = 'a', [0x74] = 'b', [0x75] = 'c', [0x76] = 'd',
    [0x77] = 'e', [0x78] = 'f', [0x79] = 'g', [0x7A] = 'h',
    [0x7B] = 'i', [0x7C] = 'j', [0x7D] = 'k', [0x7E] = 'l',
    [0x7F] = 'm', [0x80] = 'n', [0x81] = 'o', [0x82] = 'p',
    [0x83] = 'q', [0x84] = 'r', [0x85] = 's', [0x86] = 't',
    [0x87] = 'u', [0x88] = 'v', [0x89] = 'w', [0x8A] = 'x',
    [0x8B] = 'y', [0x8C] = 'z',

    [0x8F] = '1', [0x90] = '2', [0x91] = '3', [0x92] = '4',
    [0x93] = '5', [0x94] = '6', [0x95] = '7', [0x96] = '8',
    [0x97] = '9', [0x98] = '0',
};

char DORText_DecodeCharacter(uint8_t Code)
{
    if (DORTextDecodeTable[Code] != '\0') {
        return DORTextDecodeTable[Code];
    }

    return DORTextUnknownCharacter;
}

int DORText_EncodeCharacter(char Character, uint8_t* pOutCode)
{
    unsigned char Index = (unsigned char)Character;

    if (pOutCode == NULL) {
        return 0;
    }

    if (Index < sizeof(DORTextEncodeTable) && DORTextEncodeTable[Index] != 0) {
        *pOutCode = DORTextEncodeTable[Index];
        return 1;
    }

    return 0;
}

const char* DORText_Decode(const uint16_t* pEncodedText, size_t CharacterCount)
{
    static char Buffer[128];

    if (DORText_DecodeToBuffer(pEncodedText, CharacterCount, Buffer, sizeof(Buffer)) != DORStatusOk) {
        Buffer[0] = '\0';
    }

    return Buffer;
}

DORStatus DORText_DecodeToBuffer(const uint16_t* pEncodedText, size_t CharacterCount, char* pOutBuffer, size_t OutBufferSize)
{
    size_t InIndex;
    size_t OutIndex = 0;

    if (pEncodedText == NULL || pOutBuffer == NULL || OutBufferSize == 0) {
        return DORStatusInvalidArgument;
    }

    for (InIndex = 0; InIndex < CharacterCount; InIndex++) {
        uint16_t Unit = pEncodedText[InIndex];
        uint8_t Code = (uint8_t)(Unit & 0x00FFu);
        uint8_t Flags = (uint8_t)((Unit >> 8) & 0x00FFu);
        char Decoded;

        if (Code == 0) {
            break;
        }

        Decoded = DORText_DecodeCharacter(Code);
        if (OutIndex + 1u >= OutBufferSize) {
            pOutBuffer[OutIndex] = '\0';
            return DORStatusInvalidFormat;
        }

        pOutBuffer[OutIndex++] = Decoded;

        if ((Flags & 0x80u) != 0) {
            break;
        }
    }

    pOutBuffer[OutIndex] = '\0';
    return DORStatusOk;
}
