/*****************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2024 Questionable Coding
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to
* deal in the Software without restriction, including without limitation the
* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
* sell copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*
****************************************************************************/

#include <string>
#include <vector>

#include <stdio.h>

/*****************************************************************************
* This is a simple utility to ingest the whitespace-delimited tables from
* http://www.pas.rochester.edu/~emamajek/EEM_dwarf_UBVIJHK_colors_Teff.txt
* to generate a header file that contains the components that are used in
* qcSystemGenerator.
*
* This tool isn't intended to be used by anyone else, but it's included for
* completeness.
*
* The input file is expected to be found in the current working directory.
* The input table is space-delimited and ordered from hottest (O3V) to coldest
* (Y4V as of the version I developed against).  Only O3V - M9V are processed,
* and only integer values of each - the fractional entries are skipped.  The
* file is assumed to be ordered OBAFGKMLTY
*
* The output file is written in the current working directory.  You'll have to
* move it yourself to the source directory of qcSystemGenerator.  The output
* table clones the O3V entry into placeholders for O0V, O1V, and O2V that are
* used to make indexing easier at runtime.  If a future version of the source
* table includes values for those stars, I'll update this utility.
*****************************************************************************/
int main()
{
    struct StellarInfo_t
    {
        float logT; //!< log(10) of effective temperature, Kelvin.
        float logL; //!< log(10) of luminance, Sol = 0.0.
        float radius; //!< Stellar radius, in Solar radii.
        float mass; //!< Stellar mass, in Solar masses.
        std::string spectralType; //!< Keep a local copy of the type for debugging.
    };

    FILE* fp = nullptr;
    if (fopen_s(&fp, "EEM_dwarf_UBVIJHK_colors_Teff.txt", "rt"))
    {
        fprintf(stderr, "Couldn't open the file.\n");
        return __LINE__;
    }

    char lineBuffer[512];
    bool isParsing = false;

    std::string strBuffer;
    strBuffer.reserve(256);

    std::vector<std::string> tokens;
    tokens.reserve(32);

    std::vector<StellarInfo_t> stellarInfo;
    stellarInfo.reserve(70);

    while (fgets(lineBuffer, sizeof(lineBuffer), fp))
    {
        strBuffer.assign(lineBuffer);
        const int xmp = strBuffer.compare(0, 4, std::string("#SpT"));
        if (xmp == 0)
        {
            isParsing = !isParsing;
        }
        else if (strBuffer[0] == '#')
        {
            // no-op, is a comment line
        }
        else if (isParsing)
        {
            tokens.clear();

            size_t stIdx = 0;
            size_t findIdx = 0;
            while (stIdx != std::string::npos)
            {
                findIdx = strBuffer.find_first_of(' ', stIdx);
                if (findIdx != std::string::npos)
                {
                    tokens.emplace_back(strBuffer.substr(stIdx, findIdx - stIdx));
                    stIdx = strBuffer.find_first_not_of(' ', findIdx);
                }
                else
                {
                    tokens.emplace_back(strBuffer.substr(stIdx, findIdx));
                    stIdx = findIdx;
                }
            }

            if (tokens.size() < 32)
            {
                printf("Didn't get expected number of tokens parsing: %s\n", strBuffer.c_str());
            }
            else if (tokens[0][2] == '.')
            {
                printf("Skipping fractional type %s\n", tokens[0].c_str());
            }
            else if (tokens[0][0] == 'L' || tokens[0][0] == 'T' || tokens[0][0] == 'Y')
            {
                printf("Skipping unsupported type %s\n", tokens[0].c_str());
            }
            else
            {
                stellarInfo.emplace_back(StellarInfo_t
                                         {
                                        stof(tokens[2]), //float logT
                                        stof(tokens[4]), //float logL
                                        stof(tokens[6]), //float radius
                                        stof(tokens[30]), //float mass
                                        tokens[0] //std::string spectralType
                                         });
            }
        }
    }

    fclose(fp);
    fp = nullptr;

    if (fopen_s(&fp, "StellarInfo.h", "wt"))
    {
        printf("Failed to open 'StellarInfo.h' for writing\n");
        return __LINE__;
    }

    // Header boilerplate:
    fputs("/**\n", fp);
    fputs(" StellarInfo.h: auto-generated file containing the needed entries from\n", fp);
    fputs(" http://www.pas.rochester.edu/~emamajek/EEM_dwarf_UBVIJHK_colors_Teff.txt\n", fp);
    fputs("\n Table entries for O0V - O2V are cloned from O3V to provide simpler indexing\n", fp);
    fputs("\n This file is intended to be included within the qc::SystemGenerator namespace, and\n", fp);
    fputs(" not used externally from the Star class.\n", fp);
    fputs("**/\n\n", fp);
    fputs("#pragma once\n\n", fp);
    fputs("struct StellarInfo_t\n{\n", fp);
    fputs("    float logT; //!< log(10) of effective temperature, Kelvin.\n", fp);
    fputs("    float logL; //!< log(10) of luminance, Sol = 0.0.\n", fp);
    fputs("    float radius; //!< Stellar radius, in Solar radii.\n", fp);
    fputs("    float mass; //!< Stellar mass, in Solar masses.\n", fp);
    fputs("};\n\n", fp);
    fputs("static constexpr StellarInfo_t stellarInfo[] =\n{\n", fp);
    // Fake O0V - O2V:
    fprintf(fp, "    { %.3ff, %5.2ff, %6.3ff, %6.3ff }, // O0V\n",
            stellarInfo[0].logT,
            stellarInfo[0].logL,
            stellarInfo[0].radius,
            stellarInfo[0].mass);
    fprintf(fp, "    { %.3ff, %5.2ff, %6.3ff, %6.3ff }, // O1V\n",
            stellarInfo[0].logT,
            stellarInfo[0].logL,
            stellarInfo[0].radius,
            stellarInfo[0].mass);
    fprintf(fp, "    { %.3ff, %5.2ff, %6.3ff, %6.3ff }, // O2V\n",
            stellarInfo[0].logT,
            stellarInfo[0].logL,
            stellarInfo[0].radius,
            stellarInfo[0].mass);
    for (const auto& si : stellarInfo)
    {
        fprintf(fp, "    { %.3ff, %5.2ff, %6.3ff, %6.3ff }, // %s\n",
                si.logT,
                si.logL,
                si.radius,
                si.mass,
                si.spectralType.c_str());
    }
    fputs("};\n\nstatic constexpr size_t stellarInfoCount = _countof(stellarInfo);\n", fp);

    fclose(fp);

    return 0;
}
