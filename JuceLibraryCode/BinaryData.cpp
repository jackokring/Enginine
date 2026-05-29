/* ==================================== JUCER_BINARY_RESOURCE ====================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#include <cstring>

namespace BinaryData
{

//================== presets.zlib ==================
static const unsigned char temp_binary_data_0[] =
{ 120,218,77,212,203,106,27,65,16,5,208,124,138,152,125,146,234,122,55,88,246,202,249,128,60,200,58,36,131,17,216,35,99,43,198,159,31,101,49,117,107,119,7,13,28,245,84,245,189,185,123,127,122,60,188,173,47,175,167,243,118,92,198,39,90,14,235,246,251,252,
231,180,61,28,151,31,223,191,124,204,229,238,246,112,115,191,61,156,182,211,182,126,187,252,186,172,135,183,243,227,223,167,245,184,24,253,127,255,249,101,125,93,47,63,143,75,123,250,186,63,229,30,196,246,228,92,63,206,61,141,225,21,85,42,6,85,156,177,
71,102,173,104,163,98,128,162,178,68,24,127,160,52,201,210,116,148,166,90,154,122,105,58,75,51,46,205,180,52,11,156,140,24,135,44,205,173,52,207,210,98,148,22,82,90,120,105,49,75,203,81,90,106,105,25,237,67,150,54,185,180,105,165,205,196,151,36,10,100,
81,100,31,200,153,24,205,48,100,229,54,178,54,190,137,249,49,99,128,108,112,57,224,10,193,21,129,43,6,87,18,174,14,110,251,1,87,189,237,205,132,107,12,215,20,174,5,92,39,184,206,112,221,224,122,114,91,70,184,33,112,195,219,194,78,184,57,224,166,194,205,
128,155,19,238,100,184,211,184,109,126,185,76,228,200,34,200,78,200,137,171,114,29,24,178,226,178,92,15,134,60,13,87,139,185,93,51,184,28,112,133,224,138,192,21,131,43,9,87,7,92,21,184,234,112,117,114,187,211,112,77,225,90,192,117,130,235,12,215,13,174,
103,43,7,130,27,168,162,235,177,184,21,8,220,68,31,113,162,144,56,209,72,156,173,146,102,235,164,217,74,105,246,86,106,181,68,173,151,168,21,19,161,153,100,160,154,100,160,155,100,160,156,100,160,157,132,81,79,194,232,39,225,104,117,136,134,186,86,35,
92,65,71,137,160,164,68,209,82,162,123,77,125,190,253,240,15,211,249,76,181,0,0 };

const char* presets_zlib = (const char*) temp_binary_data_0;


const char* getNamedResource (const char* resourceNameUTF8, int& numBytes);
const char* getNamedResource (const char* resourceNameUTF8, int& numBytes)
{
    unsigned int hash = 0;

    if (resourceNameUTF8 != nullptr)
        while (*resourceNameUTF8 != 0)
            hash = 31 * hash + (unsigned int) *resourceNameUTF8++;

    switch (hash)
    {
        case 0x65221fb6:  numBytes = 438; return presets_zlib;
        default: break;
    }

    numBytes = 0;
    return nullptr;
}

const char* namedResourceList[] =
{
    "presets_zlib"
};

const char* originalFilenames[] =
{
    "presets.zlib"
};

const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8)
{
    for (unsigned int i = 0; i < (sizeof (namedResourceList) / sizeof (namedResourceList[0])); ++i)
        if (strcmp (namedResourceList[i], resourceNameUTF8) == 0)
            return originalFilenames[i];

    return nullptr;
}

}
