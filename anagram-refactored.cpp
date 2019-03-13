#include <stdio.h>
#include <string.h>
#include <algorithm>        // std::sort()
#include <string>           // pthread functions, setlocale(), clock_gettime()
#include <sys/sysinfo.h>    // get_nprocs()
#include <sys/mman.h>       // mmap()
#include <sys/stat.h>       // fstat()
#include <fcntl.h>          // open()
#include <unistd.h>         // close()
#include <nmmintrin.h>      // SSE/SIMD instructions

// Data struct passed into each thread
typedef struct {
    int bufferStartPos;     // First buffer byte to process by thread
    int bufferStopPos;      // Last buffer byte to process by thread
    char *frequencyTable;   // Each thread should have its own copy frequency map to operate/modify
    char *resultString;     // Here we gather suitable anagrams
    char *wordCopy;         // We want to malloc memory for dict word copy outside thread beforehand
} thread_data_t;

pthread_t *threads;
thread_data_t *threadData;

int searchWordLength;
int searchAsciiSum;
char *searchBaseAnagram;

char *dictFileContents;

// Mapping of character frequencies of search word
char *frequencyTable;

// Needle for SSE line break search, it will contain '\r' and '\0' characters
__m128i needle128;

/**
 * Robust UTF-8 to ISO-8859-13 converter
 *
 * Performing string conversions ourselves saves us from the need to use iconv
 */
static char * utfToIso(char *word)
{
    unsigned char *input = (unsigned char *)word;

    int inputLength = strlen(word);

    // We allocate with reservation (ending may remain unused)
    unsigned char *output = (unsigned char *)malloc((inputLength + 1) * sizeof(unsigned char));
    memset(output, 0, (inputLength + 1) * sizeof(unsigned char));

    int i;      // Input position tracker
    int j;      // Output position tracker
    for (i = 0, j = 0; i < inputLength; i++, j++) {
        if (input[i] <= 127) {
            output[j] = input[i];
            continue;
        }

        if (input[i] == 194) {
            if (input[i + 1] == 128) { output[j] = 128; i += 1; continue; }
            else if (input[i + 1] == 129) { output[j] = 129; i += 1; continue; }
            else if (input[i + 1] == 130) { output[j] = 130; i += 1; continue; }
            else if (input[i + 1] == 131) { output[j] = 131; i += 1; continue; }
            else if (input[i + 1] == 132) { output[j] = 132; i += 1; continue; }
            else if (input[i + 1] == 133) { output[j] = 133; i += 1; continue; }
            else if (input[i + 1] == 134) { output[j] = 134; i += 1; continue; }
            else if (input[i + 1] == 135) { output[j] = 135; i += 1; continue; }
            else if (input[i + 1] == 136) { output[j] = 136; i += 1; continue; }
            else if (input[i + 1] == 137) { output[j] = 137; i += 1; continue; }
            else if (input[i + 1] == 138) { output[j] = 138; i += 1; continue; }
            else if (input[i + 1] == 139) { output[j] = 139; i += 1; continue; }
            else if (input[i + 1] == 140) { output[j] = 140; i += 1; continue; }
            else if (input[i + 1] == 141) { output[j] = 141; i += 1; continue; }
            else if (input[i + 1] == 142) { output[j] = 142; i += 1; continue; }
            else if (input[i + 1] == 143) { output[j] = 143; i += 1; continue; }
            else if (input[i + 1] == 144) { output[j] = 144; i += 1; continue; }
            else if (input[i + 1] == 145) { output[j] = 145; i += 1; continue; }
            else if (input[i + 1] == 146) { output[j] = 146; i += 1; continue; }
            else if (input[i + 1] == 147) { output[j] = 147; i += 1; continue; }
            else if (input[i + 1] == 148) { output[j] = 148; i += 1; continue; }
            else if (input[i + 1] == 149) { output[j] = 149; i += 1; continue; }
            else if (input[i + 1] == 150) { output[j] = 150; i += 1; continue; }
            else if (input[i + 1] == 151) { output[j] = 151; i += 1; continue; }
            else if (input[i + 1] == 152) { output[j] = 152; i += 1; continue; }
            else if (input[i + 1] == 153) { output[j] = 153; i += 1; continue; }
            else if (input[i + 1] == 154) { output[j] = 154; i += 1; continue; }
            else if (input[i + 1] == 155) { output[j] = 155; i += 1; continue; }
            else if (input[i + 1] == 156) { output[j] = 156; i += 1; continue; }
            else if (input[i + 1] == 157) { output[j] = 157; i += 1; continue; }
            else if (input[i + 1] == 158) { output[j] = 158; i += 1; continue; }
            else if (input[i + 1] == 159) { output[j] = 159; i += 1; continue; }
            else if (input[i + 1] == 160) { output[j] = 160; i += 1; continue; }
            else if (input[i + 1] == 162) { output[j] = 162; i += 1; continue; }
            else if (input[i + 1] == 163) { output[j] = 163; i += 1; continue; }
            else if (input[i + 1] == 164) { output[j] = 164; i += 1; continue; }
            else if (input[i + 1] == 166) { output[j] = 166; i += 1; continue; }
            else if (input[i + 1] == 167) { output[j] = 167; i += 1; continue; }
            else if (input[i + 1] == 169) { output[j] = 169; i += 1; continue; }
            else if (input[i + 1] == 171) { output[j] = 171; i += 1; continue; }
            else if (input[i + 1] == 172) { output[j] = 172; i += 1; continue; }
            else if (input[i + 1] == 173) { output[j] = 173; i += 1; continue; }
            else if (input[i + 1] == 174) { output[j] = 174; i += 1; continue; }
            else if (input[i + 1] == 176) { output[j] = 176; i += 1; continue; }
            else if (input[i + 1] == 177) { output[j] = 177; i += 1; continue; }
            else if (input[i + 1] == 178) { output[j] = 178; i += 1; continue; }
            else if (input[i + 1] == 179) { output[j] = 179; i += 1; continue; }
            else if (input[i + 1] == 181) { output[j] = 181; i += 1; continue; }
            else if (input[i + 1] == 182) { output[j] = 182; i += 1; continue; }
            else if (input[i + 1] == 183) { output[j] = 183; i += 1; continue; }
            else if (input[i + 1] == 185) { output[j] = 185; i += 1; continue; }
            else if (input[i + 1] == 187) { output[j] = 187; i += 1; continue; }
            else if (input[i + 1] == 188) { output[j] = 188; i += 1; continue; }
            else if (input[i + 1] == 189) { output[j] = 189; i += 1; continue; }
            else if (input[i + 1] == 190) { output[j] = 190; i += 1; continue; }
            else { return (char *)""; }     // Invalid utf->iso conversion
        } else if (input[i] == 195) {
            if (input[i + 1] == 132) { output[j] = 196; i += 1; continue; }
            else if (input[i + 1] == 133) { output[j] = 197; i += 1; continue; }
            else if (input[i + 1] == 134) { output[j] = 175; i += 1; continue; }
            else if (input[i + 1] == 137) { output[j] = 201; i += 1; continue; }
            else if (input[i + 1] == 147) { output[j] = 211; i += 1; continue; }
            else if (input[i + 1] == 149) { output[j] = 213; i += 1; continue; }
            else if (input[i + 1] == 150) { output[j] = 214; i += 1; continue; }
            else if (input[i + 1] == 151) { output[j] = 215; i += 1; continue; }
            else if (input[i + 1] == 152) { output[j] = 168; i += 1; continue; }
            else if (input[i + 1] == 156) { output[j] = 220; i += 1; continue; }
            else if (input[i + 1] == 159) { output[j] = 223; i += 1; continue; }
            else if (input[i + 1] == 164) { output[j] = 228; i += 1; continue; }
            else if (input[i + 1] == 165) { output[j] = 229; i += 1; continue; }
            else if (input[i + 1] == 166) { output[j] = 191; i += 1; continue; }
            else if (input[i + 1] == 169) { output[j] = 233; i += 1; continue; }
            else if (input[i + 1] == 179) { output[j] = 243; i += 1; continue; }
            else if (input[i + 1] == 181) { output[j] = 245; i += 1; continue; }
            else if (input[i + 1] == 182) { output[j] = 246; i += 1; continue; }
            else if (input[i + 1] == 183) { output[j] = 247; i += 1; continue; }
            else if (input[i + 1] == 184) { output[j] = 184; i += 1; continue; }
            else if (input[i + 1] == 188) { output[j] = 252; i += 1; continue; }
            else { return (char *)""; }     // Invalid utf->iso conversion
        } else if (input[i] == 196) {
            if (input[i + 1] == 128) { output[j] = 194; i += 1; continue; }
            else if (input[i + 1] == 129) { output[j] = 226; i += 1; continue; }
            else if (input[i + 1] == 132) { output[j] = 192; i += 1; continue; }
            else if (input[i + 1] == 133) { output[j] = 224; i += 1; continue; }
            else if (input[i + 1] == 134) { output[j] = 195; i += 1; continue; }
            else if (input[i + 1] == 135) { output[j] = 227; i += 1; continue; }
            else if (input[i + 1] == 140) { output[j] = 200; i += 1; continue; }
            else if (input[i + 1] == 141) { output[j] = 232; i += 1; continue; }
            else if (input[i + 1] == 146) { output[j] = 199; i += 1; continue; }
            else if (input[i + 1] == 147) { output[j] = 231; i += 1; continue; }
            else if (input[i + 1] == 150) { output[j] = 203; i += 1; continue; }
            else if (input[i + 1] == 151) { output[j] = 235; i += 1; continue; }
            else if (input[i + 1] == 152) { output[j] = 198; i += 1; continue; }
            else if (input[i + 1] == 153) { output[j] = 230; i += 1; continue; }
            else if (input[i + 1] == 162) { output[j] = 204; i += 1; continue; }
            else if (input[i + 1] == 163) { output[j] = 236; i += 1; continue; }
            else if (input[i + 1] == 170) { output[j] = 206; i += 1; continue; }
            else if (input[i + 1] == 171) { output[j] = 238; i += 1; continue; }
            else if (input[i + 1] == 174) { output[j] = 193; i += 1; continue; }
            else if (input[i + 1] == 175) { output[j] = 225; i += 1; continue; }
            else if (input[i + 1] == 182) { output[j] = 205; i += 1; continue; }
            else if (input[i + 1] == 183) { output[j] = 237; i += 1; continue; }
            else if (input[i + 1] == 187) { output[j] = 207; i += 1; continue; }
            else if (input[i + 1] == 188) { output[j] = 239; i += 1; continue; }
            else { return (char *)""; }     // Invalid utf->iso conversion
        } else if (input[i] == 197) {
            if (input[i + 1] == 129) { output[j] = 217; i += 1; continue; }
            else if (input[i + 1] == 130) { output[j] = 249; i += 1; continue; }
            else if (input[i + 1] == 131) { output[j] = 209; i += 1; continue; }
            else if (input[i + 1] == 132) { output[j] = 241; i += 1; continue; }
            else if (input[i + 1] == 133) { output[j] = 210; i += 1; continue; }
            else if (input[i + 1] == 134) { output[j] = 242; i += 1; continue; }
            else if (input[i + 1] == 140) { output[j] = 212; i += 1; continue; }
            else if (input[i + 1] == 141) { output[j] = 244; i += 1; continue; }
            else if (input[i + 1] == 150) { output[j] = 170; i += 1; continue; }
            else if (input[i + 1] == 151) { output[j] = 186; i += 1; continue; }
            else if (input[i + 1] == 154) { output[j] = 218; i += 1; continue; }
            else if (input[i + 1] == 155) { output[j] = 250; i += 1; continue; }
            else if (input[i + 1] == 160) { output[j] = 208; i += 1; continue; }
            else if (input[i + 1] == 161) { output[j] = 240; i += 1; continue; }
            else if (input[i + 1] == 170) { output[j] = 219; i += 1; continue; }
            else if (input[i + 1] == 171) { output[j] = 251; i += 1; continue; }
            else if (input[i + 1] == 178) { output[j] = 216; i += 1; continue; }
            else if (input[i + 1] == 179) { output[j] = 248; i += 1; continue; }
            else if (input[i + 1] == 185) { output[j] = 202; i += 1; continue; }
            else if (input[i + 1] == 186) { output[j] = 234; i += 1; continue; }
            else if (input[i + 1] == 187) { output[j] = 221; i += 1; continue; }
            else if (input[i + 1] == 188) { output[j] = 253; i += 1; continue; }
            else if (input[i + 1] == 189) { output[j] = 222; i += 1; continue; }
            else if (input[i + 1] == 190) { output[j] = 254; i += 1; continue; }
            else { return (char *)""; }     // Invalid utf->iso conversion
        } else if (input[i] == 226) {
            if (input[i + 1] == 128 && input[i + 2] == 153) { output[j] = 255; i += 2; continue; }
            else if (input[i + 1] == 128 && input[i + 2] == 156) { output[j] = 180; i += 2; continue; }
            else if (input[i + 1] == 128 && input[i + 2] == 157) { output[j] = 161; i += 2; continue; }
            else if (input[i + 1] == 128 && input[i + 2] == 158) { output[j] = 165; i += 2; continue; }
            else { return (char *)""; }     // Invalid utf->iso conversion
        } else {
            return (char *)"";  // Invalid utf->iso conversion
        }
    }

    return (char *)output;
}

/**
 * Robust ISO-8859-13 to UTF-8 converter
 *
 * Performing string conversions ourselves saves us from the need to use iconv
 */
static char * isoToUtf(char *word)
{
    unsigned char *input = (unsigned char *)word;

    int inputLength = strlen(word);

    // Single iso char may be up to 3 bytes in utf so we allocate 3*isolength bytes
    unsigned char *output = (unsigned char *)malloc((inputLength + 1) * sizeof(unsigned char) * 3);
    memset(output, 0, (inputLength + 1) * sizeof(unsigned char) * 3);

    int i;      // Input position tracker
    int j;      // Output position tracker
    for (i = 0, j = 0; i < inputLength; i++, j++) {
        if (input[i] <= 127) {
            output[j] = input[i];
            continue;
        }

        // Lets convert some more popular letters first
        if (input[i] == 228) { output[j] = 195; output[j + 1] = 164; j += 1; continue; }    // ä
        if (input[i] == 246) { output[j] = 195; output[j + 1] = 182; j += 1; continue; }    // ö
        if (input[i] == 252) { output[j] = 195; output[j + 1] = 188; j += 1; continue; }    // ü
        if (input[i] == 244) { output[j] = 197; output[j + 1] = 141; j += 1; continue; }    // õ
        if (input[i] == 196) { output[j] = 195; output[j + 1] = 132; j += 1; continue; }    // Ä
        if (input[i] == 214) { output[j] = 195; output[j + 1] = 150; j += 1; continue; }    // Ö
        if (input[i] == 220) { output[j] = 195; output[j + 1] = 156; j += 1; continue; }    // Ü
        if (input[i] == 213) { output[j] = 195; output[j + 1] = 149; j += 1; continue; }    // Õ

        if (input[i] == 128) { output[j] = 194; output[j + 1] = 128; j += 1; continue; }
        if (input[i] == 129) { output[j] = 194; output[j + 1] = 129; j += 1; continue; }
        if (input[i] == 130) { output[j] = 194; output[j + 1] = 130; j += 1; continue; }
        if (input[i] == 131) { output[j] = 194; output[j + 1] = 131; j += 1; continue; }
        if (input[i] == 132) { output[j] = 194; output[j + 1] = 132; j += 1; continue; }
        if (input[i] == 133) { output[j] = 194; output[j + 1] = 133; j += 1; continue; }
        if (input[i] == 134) { output[j] = 194; output[j + 1] = 134; j += 1; continue; }
        if (input[i] == 135) { output[j] = 194; output[j + 1] = 135; j += 1; continue; }
        if (input[i] == 136) { output[j] = 194; output[j + 1] = 136; j += 1; continue; }
        if (input[i] == 137) { output[j] = 194; output[j + 1] = 137; j += 1; continue; }
        if (input[i] == 138) { output[j] = 194; output[j + 1] = 138; j += 1; continue; }
        if (input[i] == 139) { output[j] = 194; output[j + 1] = 139; j += 1; continue; }
        if (input[i] == 140) { output[j] = 194; output[j + 1] = 140; j += 1; continue; }
        if (input[i] == 141) { output[j] = 194; output[j + 1] = 141; j += 1; continue; }
        if (input[i] == 142) { output[j] = 194; output[j + 1] = 142; j += 1; continue; }
        if (input[i] == 143) { output[j] = 194; output[j + 1] = 143; j += 1; continue; }
        if (input[i] == 144) { output[j] = 194; output[j + 1] = 144; j += 1; continue; }
        if (input[i] == 145) { output[j] = 194; output[j + 1] = 145; j += 1; continue; }
        if (input[i] == 146) { output[j] = 194; output[j + 1] = 146; j += 1; continue; }
        if (input[i] == 147) { output[j] = 194; output[j + 1] = 147; j += 1; continue; }
        if (input[i] == 148) { output[j] = 194; output[j + 1] = 148; j += 1; continue; }
        if (input[i] == 149) { output[j] = 194; output[j + 1] = 149; j += 1; continue; }
        if (input[i] == 150) { output[j] = 194; output[j + 1] = 150; j += 1; continue; }
        if (input[i] == 151) { output[j] = 194; output[j + 1] = 151; j += 1; continue; }
        if (input[i] == 152) { output[j] = 194; output[j + 1] = 152; j += 1; continue; }
        if (input[i] == 153) { output[j] = 194; output[j + 1] = 153; j += 1; continue; }
        if (input[i] == 154) { output[j] = 194; output[j + 1] = 154; j += 1; continue; }
        if (input[i] == 155) { output[j] = 194; output[j + 1] = 155; j += 1; continue; }
        if (input[i] == 156) { output[j] = 194; output[j + 1] = 156; j += 1; continue; }
        if (input[i] == 157) { output[j] = 194; output[j + 1] = 157; j += 1; continue; }
        if (input[i] == 158) { output[j] = 194; output[j + 1] = 158; j += 1; continue; }
        if (input[i] == 159) { output[j] = 194; output[j + 1] = 159; j += 1; continue; }
        if (input[i] == 160) { output[j] = 194; output[j + 1] = 160; j += 1; continue; }
        if (input[i] == 161) { output[j] = 226; output[j + 1] = 128; output[j + 2] = 157; j += 2; continue; }
        if (input[i] == 162) { output[j] = 194; output[j + 1] = 162; j += 1; continue; }
        if (input[i] == 163) { output[j] = 194; output[j + 1] = 163; j += 1; continue; }
        if (input[i] == 164) { output[j] = 194; output[j + 1] = 164; j += 1; continue; }
        if (input[i] == 165) { output[j] = 226; output[j + 1] = 128; output[j + 2] = 158; j += 2; continue; }
        if (input[i] == 166) { output[j] = 194; output[j + 1] = 166; j += 1; continue; }
        if (input[i] == 167) { output[j] = 194; output[j + 1] = 167; j += 1; continue; }
        if (input[i] == 168) { output[j] = 195; output[j + 1] = 152; j += 1; continue; }
        if (input[i] == 169) { output[j] = 194; output[j + 1] = 169; j += 1; continue; }
        if (input[i] == 170) { output[j] = 197; output[j + 1] = 150; j += 1; continue; }
        if (input[i] == 171) { output[j] = 194; output[j + 1] = 171; j += 1; continue; }
        if (input[i] == 172) { output[j] = 194; output[j + 1] = 172; j += 1; continue; }
        if (input[i] == 173) { output[j] = 194; output[j + 1] = 173; j += 1; continue; }
        if (input[i] == 174) { output[j] = 194; output[j + 1] = 174; j += 1; continue; }
        if (input[i] == 175) { output[j] = 195; output[j + 1] = 134; j += 1; continue; }
        if (input[i] == 176) { output[j] = 194; output[j + 1] = 176; j += 1; continue; }
        if (input[i] == 177) { output[j] = 194; output[j + 1] = 177; j += 1; continue; }
        if (input[i] == 178) { output[j] = 194; output[j + 1] = 178; j += 1; continue; }
        if (input[i] == 179) { output[j] = 194; output[j + 1] = 179; j += 1; continue; }
        if (input[i] == 180) { output[j] = 226; output[j + 1] = 128; output[j + 2] = 156; j += 2; continue; }
        if (input[i] == 181) { output[j] = 194; output[j + 1] = 181; j += 1; continue; }
        if (input[i] == 182) { output[j] = 194; output[j + 1] = 182; j += 1; continue; }
        if (input[i] == 183) { output[j] = 194; output[j + 1] = 183; j += 1; continue; }
        if (input[i] == 184) { output[j] = 195; output[j + 1] = 184; j += 1; continue; }
        if (input[i] == 185) { output[j] = 194; output[j + 1] = 185; j += 1; continue; }
        if (input[i] == 186) { output[j] = 197; output[j + 1] = 151; j += 1; continue; }
        if (input[i] == 187) { output[j] = 194; output[j + 1] = 187; j += 1; continue; }
        if (input[i] == 188) { output[j] = 194; output[j + 1] = 188; j += 1; continue; }
        if (input[i] == 189) { output[j] = 194; output[j + 1] = 189; j += 1; continue; }
        if (input[i] == 190) { output[j] = 194; output[j + 1] = 190; j += 1; continue; }
        if (input[i] == 191) { output[j] = 195; output[j + 1] = 166; j += 1; continue; }
        if (input[i] == 192) { output[j] = 196; output[j + 1] = 132; j += 1; continue; }
        if (input[i] == 193) { output[j] = 196; output[j + 1] = 174; j += 1; continue; }
        if (input[i] == 194) { output[j] = 196; output[j + 1] = 128; j += 1; continue; }
        if (input[i] == 195) { output[j] = 196; output[j + 1] = 134; j += 1; continue; }
        if (input[i] == 197) { output[j] = 195; output[j + 1] = 133; j += 1; continue; }
        if (input[i] == 198) { output[j] = 196; output[j + 1] = 152; j += 1; continue; }
        if (input[i] == 199) { output[j] = 196; output[j + 1] = 146; j += 1; continue; }
        if (input[i] == 200) { output[j] = 196; output[j + 1] = 140; j += 1; continue; }
        if (input[i] == 201) { output[j] = 195; output[j + 1] = 137; j += 1; continue; }
        if (input[i] == 202) { output[j] = 197; output[j + 1] = 185; j += 1; continue; }
        if (input[i] == 203) { output[j] = 196; output[j + 1] = 150; j += 1; continue; }
        if (input[i] == 204) { output[j] = 196; output[j + 1] = 162; j += 1; continue; }
        if (input[i] == 205) { output[j] = 196; output[j + 1] = 182; j += 1; continue; }
        if (input[i] == 206) { output[j] = 196; output[j + 1] = 170; j += 1; continue; }
        if (input[i] == 207) { output[j] = 196; output[j + 1] = 187; j += 1; continue; }
        if (input[i] == 208) { output[j] = 197; output[j + 1] = 160; j += 1; continue; }
        if (input[i] == 209) { output[j] = 197; output[j + 1] = 131; j += 1; continue; }
        if (input[i] == 210) { output[j] = 197; output[j + 1] = 133; j += 1; continue; }
        if (input[i] == 211) { output[j] = 195; output[j + 1] = 147; j += 1; continue; }
        if (input[i] == 212) { output[j] = 197; output[j + 1] = 140; j += 1; continue; }
        if (input[i] == 215) { output[j] = 195; output[j + 1] = 151; j += 1; continue; }
        if (input[i] == 216) { output[j] = 197; output[j + 1] = 178; j += 1; continue; }
        if (input[i] == 217) { output[j] = 197; output[j + 1] = 129; j += 1; continue; }
        if (input[i] == 218) { output[j] = 197; output[j + 1] = 154; j += 1; continue; }
        if (input[i] == 219) { output[j] = 197; output[j + 1] = 170; j += 1; continue; }
        if (input[i] == 221) { output[j] = 197; output[j + 1] = 187; j += 1; continue; }
        if (input[i] == 222) { output[j] = 197; output[j + 1] = 189; j += 1; continue; }
        if (input[i] == 223) { output[j] = 195; output[j + 1] = 159; j += 1; continue; }
        if (input[i] == 224) { output[j] = 196; output[j + 1] = 133; j += 1; continue; }
        if (input[i] == 225) { output[j] = 196; output[j + 1] = 175; j += 1; continue; }
        if (input[i] == 226) { output[j] = 196; output[j + 1] = 129; j += 1; continue; }
        if (input[i] == 227) { output[j] = 196; output[j + 1] = 135; j += 1; continue; }
        if (input[i] == 229) { output[j] = 195; output[j + 1] = 165; j += 1; continue; }
        if (input[i] == 230) { output[j] = 196; output[j + 1] = 153; j += 1; continue; }
        if (input[i] == 231) { output[j] = 196; output[j + 1] = 147; j += 1; continue; }
        if (input[i] == 232) { output[j] = 196; output[j + 1] = 141; j += 1; continue; }
        if (input[i] == 233) { output[j] = 195; output[j + 1] = 169; j += 1; continue; }
        if (input[i] == 234) { output[j] = 197; output[j + 1] = 186; j += 1; continue; }
        if (input[i] == 235) { output[j] = 196; output[j + 1] = 151; j += 1; continue; }
        if (input[i] == 236) { output[j] = 196; output[j + 1] = 163; j += 1; continue; }
        if (input[i] == 237) { output[j] = 196; output[j + 1] = 183; j += 1; continue; }
        if (input[i] == 238) { output[j] = 196; output[j + 1] = 171; j += 1; continue; }
        if (input[i] == 239) { output[j] = 196; output[j + 1] = 188; j += 1; continue; }
        if (input[i] == 240) { output[j] = 197; output[j + 1] = 161; j += 1; continue; }
        if (input[i] == 241) { output[j] = 197; output[j + 1] = 132; j += 1; continue; }
        if (input[i] == 242) { output[j] = 197; output[j + 1] = 134; j += 1; continue; }
        if (input[i] == 243) { output[j] = 195; output[j + 1] = 179; j += 1; continue; }
        if (input[i] == 245) { output[j] = 195; output[j + 1] = 181; j += 1; continue; }
        if (input[i] == 247) { output[j] = 195; output[j + 1] = 183; j += 1; continue; }
        if (input[i] == 248) { output[j] = 197; output[j + 1] = 179; j += 1; continue; }
        if (input[i] == 249) { output[j] = 197; output[j + 1] = 130; j += 1; continue; }
        if (input[i] == 250) { output[j] = 197; output[j + 1] = 155; j += 1; continue; }
        if (input[i] == 251) { output[j] = 197; output[j + 1] = 171; j += 1; continue; }
        if (input[i] == 253) { output[j] = 197; output[j + 1] = 188; j += 1; continue; }
        if (input[i] == 254) { output[j] = 197; output[j + 1] = 190; j += 1; continue; }
        if (input[i] == 255) { output[j] = 226; output[j + 1] = 128; output[j + 2] = 153; j += 2; continue; }
    }

    return (char *)output;
}

/**
 * Check whether the word contains extended characters
 *
 * Finding any characters with the ASCII code >127 means that we have a word
 * with extended characters ('õ', 'ä', 'ö', 'ü', etc)
 */
static bool hasExtendedChars(char *word)
{
    unsigned char *input = (unsigned char *)word;
    int inputLength = strlen(word);

    int i;
    for (i = 0; i < inputLength; i++) {
        if (input[i] <= 127) {
            continue;
        }

        return true;
    }

    return false;
}

/**
 * Return "base" anagram for the word
 *
 * Sorts letters in a word by ASCII code, for example anagrams 'foo', 'ofo' and
 * 'oof' will all have the same "base" anagram of 'foo'
 */
static char * getBaseAnagram(char *word)
{
    std::sort(word, word + strlen(word));

    return word;
}

/**
 * Find line break in buffer
 *
 * Search for '\r' character indicating word ending. We use SSE instructions to
 * check 16 characters at once to be as fast as possible.
 */
inline static char *findLineBreak(char *input)
{
    char *haystack = input;
    __m128i haystack128;
    int foundPos;

    while (true) {
        haystack128 = _mm_loadu_si128((const __m128i *)(haystack));

        // SSE instruction returning position of the first match of any of the
        // characters provided by needle
        foundPos = _mm_cmpestri(needle128, 1, haystack128, 16, _SIDD_CMP_EQUAL_ANY);

        // If foundPos is 16 then no match was found and we should shift pointer
        // forward by 16 characters and continue searching
        if (foundPos & 0x10) {
            haystack += 16;
        } else {
            return haystack + foundPos;
        }
    }
}

/**
 * Calculate ASCII sum of all the characters in the word
 */
inline static int getAsciiSum(char *word, int length)
{
    unsigned char *letter = (unsigned char *)word;

    int asciiSum = 0;

    for (int i = 0; i < length; i++) {
        asciiSum += letter[i];
    }

    return asciiSum;
}

/**
 * Anagram finder thread
 *
 * Main workhorse of the code. We will run number of processors worker threads
 * in parallel to get best performance. Each thread will work through part of
 * dictionary file searching for anagrams.
 *
 * IMPORTANT: Don't malloc anything here in thread, do it all beforehand.
 */
static void * anagramFinderThread(void * inputDataRaw)
{
    thread_data_t *inputData = (thread_data_t *)inputDataRaw;

    char *returnPosition = dictFileContents + inputData->bufferStopPos;
    char *dictWordStart = dictFileContents + inputData->bufferStartPos;
    char *dictWordEnd;
    int dictWordLength;

    // Keeping track of result string length frees us from the need of using
    // unnecessary strlen when copying found anagram to result string
    int resultStringLength = 0;

    while (dictWordStart < returnPosition) {
        dictWordEnd = findLineBreak(dictWordStart);

        // Most basic filtering out of unsuitable words - if string lengths
        // differ then we cannot have an anagram
        if ((dictWordEnd - dictWordStart) != searchWordLength) {
            dictWordStart = dictWordEnd + 2;    // Skip to the start of the next word
            continue;
        }

        dictWordLength = dictWordEnd - dictWordStart;

        // If strings are of equal length then check whether sum of ASCII codes
        // of the letters in dictionary word equals the one of the search word -
        // this should keep us with very few words that we need to perform the
        // final (costlier) std::sort on.
        if (getAsciiSum(dictWordStart, dictWordLength) != searchAsciiSum) {
            dictWordStart = dictWordEnd + 2;    // Skip to the start of the next word
            continue;
        }

        // Since std::sort operates with input directly, we need to pass a copy
        // of the word to the sorting algorithm, otherwise we cannot print the
        // word out later as it will have characters in the wrong order
        memcpy(inputData->wordCopy, dictWordStart, dictWordLength);
        inputData->wordCopy[dictWordLength] = '\0';
        std::sort(inputData->wordCopy, inputData->wordCopy + dictWordLength);

        // Here we finally check whether dict word is an anagram for search word
        if (strcmp(inputData->wordCopy, searchBaseAnagram) != 0) {
            dictWordStart = dictWordEnd + 2;    // Skip to the start of the next word
            continue;
        }

        // And reaching this point after passing all the checks means that we
        // have found a suitable anagram - we can add the word to our results
        // string
        inputData->resultString[resultStringLength] = ',';
        memcpy(inputData->resultString + resultStringLength + 1, dictWordStart, dictWordLength);
        resultStringLength += dictWordLength + 1;   // +1 stands for comma

        dictWordStart = dictWordEnd + 2;    // Skip to the start of the next word
    }

    // Finish result string properly
    inputData->resultString[resultStringLength] = '\0';

    return NULL;
}


int main(int argc, char **argv)
{
    // Start time tracking
    timespec startTime;
    clock_gettime(CLOCK_REALTIME, &startTime);


    // STEP 1: Read cmd line arguments

    // Validate number of arguments
    if (argc != 3) {
        printf("ERROR: Two arguments expected (%s [fullPathToDictionaryFile] [wordToFindAnagramsFor])\n", argv[0]);
        return 1;
    }

    // Without setting locale results may not be printed out correctly in case
    // search word/anagrams contain extended chars
    setlocale(LC_ALL, "en_US.UTF-8");

    char *dictFilename = argv[1];
    char *searchWordUtf = argv[2];

    char *searchWord = utfToIso(searchWordUtf);
    searchWordLength = strlen(searchWord);
    searchAsciiSum = getAsciiSum(searchWord, searchWordLength);
    searchBaseAnagram = getBaseAnagram(searchWord);
    bool searchHasExtendedChars = hasExtendedChars(searchWord);

    // Return early if empty or invalid (non-utf-to-iso-convertible) search
    // provided (for example 'þürii' as 'þ' is missing in iso 8859-13 so it
    // is non-convertible for us)
    if (searchWordLength <= 0) {
        // Stop time tracking...
        timespec stopTime;
        clock_gettime(CLOCK_REALTIME, &stopTime);

        // ...and print the results
        printf("%.0f\n", ((stopTime.tv_sec - startTime.tv_sec) * 1000000) + (stopTime.tv_nsec - startTime.tv_nsec) / 1000.);
        return 0;
    }

    // Calculate how many times each letter is present in search word
    frequencyTable = (char *)malloc(256 * sizeof(char));
    for (int j = 0; j < searchWordLength; j++) {
        frequencyTable[(unsigned char)searchWord[j]]++;
    }


    // STEP 2: Read input file

    // Open file
    int dictFilePtr = open(dictFilename, O_RDONLY | O_NOATIME);
    if (dictFilePtr == -1) {
        printf("ERROR: Cannot open file %s\n", dictFilename);
        return 1;
    }

    // Get some stats about file size etc
    struct stat dictFileStats;
    fstat(dictFilePtr, &dictFileStats);
    int dictFileSize = dictFileStats.st_size;

    // Mmap file
    dictFileContents = (char *)mmap(0, dictFileSize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, dictFilePtr, 0);
    close(dictFilePtr);


    // STEP 3: Prepare data for multithreading

    int numProcs = get_nprocs();

    threads = (pthread_t *)malloc(numProcs * sizeof(pthread_t));
    threadData = (thread_data_t *)malloc(numProcs * sizeof(thread_data_t));

    // Init SSE search needle with '\r' for fast line break detection
    char needle[16];
    strcpy(needle, "\r\0");
    needle[15] = '\0';
    needle128 = _mm_loadu_si128((const __m128i *)(needle));

    int threadProcessAmount = dictFileSize / numProcs;
    int startPos = 0;
    int stopPos = 0;

    // Prepare some numbers for each thread: calculate at what position it
    // should start and stop processing dictionary file and make copy of
    // frequency table so each thread can tamper it without affecting others
    for (int i = 0; i < numProcs; i++) {
        if (i == 0) {
            startPos = 0;
        } else {
            startPos = stopPos + 2;     // Calculated form prev thread end pos
        }

        if (i == numProcs - 1) {
            stopPos = dictFileSize;     // Last thread ends at file end
        } else {
            char *endMarker = findLineBreak(dictFileContents + ((i + 1) * threadProcessAmount) - 1);
            stopPos = endMarker - dictFileContents;
        }

        // Set buffer first and last position to process for each thread
        threadData[i].bufferStartPos = startPos;
        threadData[i].bufferStopPos = stopPos;

        // Copy search word frequency map for each thread
        threadData[i].frequencyTable = (char *)malloc(256 * sizeof(char));
        memcpy(threadData[i].frequencyTable, frequencyTable, 256 * sizeof(char));

        // IMPORTANT: Malloc all the necessary memory for threads beforehand in
        // main thread, we experience significant loss of performance in case we
        // have multiple threads doing even simplest of mallocing in parallel
        // (observed on a machine with 8 processors, but in the same time there
        // was no difference on machine with 4 processors between mallocing
        // inside and outside of threads)
        threadData[i].resultString = (char *)malloc(1024 * sizeof(char));
        threadData[i].wordCopy = (char *)malloc(1024 * sizeof(char));
    }

    // 4) Search for anagrams in multiple threads in parallel

    // Open numProcs-1 threads
    for (int i = 1; i < numProcs; i++) {
        pthread_create(&threads[i], NULL, (void *(*)(void *))anagramFinderThread, &threadData[i]);
    }

    // Process data also in main thread
    anagramFinderThread(&threadData[0]);

    // Close opened threads once they have finished processing
    for (int i = 1; i < numProcs; i++) {
        pthread_join(threads[i], NULL);
    }

    char *resultString = (char *)malloc(4096 * sizeof(char));
    for (int i = 0; i < numProcs; i++) {
        strncat(resultString, threadData[i].resultString, strlen(threadData[i].resultString) + 1);
    }

    char *resultStringUtf;

    if (searchHasExtendedChars) {
        resultStringUtf = isoToUtf(resultString);
    } else {
        resultStringUtf = resultString;
    }

    // Stop time tracking...
    timespec stopTime;
    clock_gettime(CLOCK_REALTIME, &stopTime);

    // ...and print the results
    printf("%.0f%s\n", ((stopTime.tv_sec - startTime.tv_sec) * 1000000) + (stopTime.tv_nsec - startTime.tv_nsec) / 1000., resultStringUtf);
    // printf("%.0f\n", ((stopTime.tv_sec - startTime.tv_sec) * 1000000) + (stopTime.tv_nsec - startTime.tv_nsec) / 1000.);

    return 0;
}