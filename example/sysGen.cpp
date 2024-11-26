/*****************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2021-2024 Questionable Coding
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
#include <qcSysGen/System.h>

#include <qcSysGen/Generator.h>

#include <assert.h>
#include <string>

using namespace qc::SystemGenerator;

//----------------------------------------------------------------------------
void SG_Callback(const char* text)
{
    printf(text);
}

//----------------------------------------------------------------------------
std::string RomanNumeral(int value)
{
    static const char* tens[] = { "", "X", "XX", "XXX", "XL", "L", "LX", "LXX", "LXXX", "XC" };
    static const char* ones[] = { "", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX" };

    const int tDigit = (value / 10) % 10;
    const int oDigit = (value % 10);

    std::string str(tens[tDigit]);
    str.append(ones[oDigit]);

    return str;
}

//----------------------------------------------------------------------------
std::string PlanetOrdinal(int planetOrd)
{
    std::string str(RomanNumeral(planetOrd));
    if (str.length() < 7u)
    {
        str.append(7u - str.length(), ' ');
    }

    return str;
}

//----------------------------------------------------------------------------
std::string MoonOrdinal(int planetOrd, int moonOrd)
{
    std::string str(" ");
    str.append(RomanNumeral(planetOrd));
    char s[4];
    sprintf_s(s, "%c", 'a' + moonOrd - 1);
    str.append(s);
    if (str.length() < 8u)
    {
        str.append(8u - str.length(), ' ');
    }

    return str;
}

//----------------------------------------------------------------------------
char AsciiArtType(PlanetType type)
{
    switch (type)
    {
        case PlanetType::IceGiant:
            return 'o';

        case PlanetType::GasGiant:
            return 'O';

        case PlanetType::AsteroidBelt:
            return ':';
            //case PlanetType::Goldilocks:
            //    return 'G';
        default:
        case PlanetType::Rocky:
            return '.';
    }
}

//----------------------------------------------------------------------------
std::string SMA(double sma)
{
    char smaStr[16];

    sprintf_s(smaStr, "%7.3lfAU", sma);
    return std::string(smaStr);
}

//----------------------------------------------------------------------------
std::string km(double sma)
{
    char smaStr[16];

    sprintf_s(smaStr, "%8.0lfkm  ", sma * KmPerAu);
    return std::string(smaStr);
}

//----------------------------------------------------------------------------
std::string Radius(double radius)
{
    char radiusStr[16];

    sprintf_s(radiusStr, "%9.0lfkm ", radius);
    return std::string(radiusStr);
}

//----------------------------------------------------------------------------
std::string MeanSurfaceConditions(float surfaceTempCelsius, float surfacePressureMb)
{
    char tempStr[64];

    sprintf_s(tempStr, "%4.0f*C %7.3fatm ", surfaceTempCelsius, surfacePressureMb / EarthSurfacePressureMb);
    return std::string(tempStr);
}

//----------------------------------------------------------------------------
void SpewPlanet(const Planet& pl, int planetOrdinal)
{
    std::string planetInfo;

    planetInfo.append(PlanetOrdinal(planetOrdinal)).append(" - ").append(pl.getName());
    puts(planetInfo.c_str());
    planetInfo.clear();

    planetInfo.append("\t").append(PlanetTypeName(pl.getPlanetType())).append(" - ").append(OrbitalZoneName(pl.getOrbitalZone())).append(" Zone");
    puts(planetInfo.c_str());
    planetInfo.clear();

    if (pl.getPlanetType() != PlanetType::AsteroidBelt)
    {
        char buffer[64];

        planetInfo.append("\tSemi-major axis: ");
        planetInfo.append(SMA(pl.getSemimajorAxis()));
        planetInfo.append(" (Pe: ").append(SMA(pl.getPeriapsis())).append(", Ap: ").append(SMA(pl.getApoapsis())).append(")");
        puts(planetInfo.c_str());
        planetInfo.clear();

        sprintf_s(buffer, "%.3f", pl.getEccentricity());
        planetInfo.append("\tecc: ").append(buffer).append(", inc: ");
        sprintf_s(buffer, "%.2f*", pl.getInclination());
        planetInfo.append(buffer);
        puts(planetInfo.c_str());
        planetInfo.clear();

        planetInfo.append("\tRadius: ").append(Radius(pl.getRadius()));
        puts(planetInfo.c_str());
        planetInfo.clear();

        sprintf_s(buffer, "%4.1fg/cc", pl.getDensity());
        planetInfo.append("\tDensity: ").append(buffer);
        puts(planetInfo.c_str());
        planetInfo.clear();

        if (!pl.isGaseous())
        {
            sprintf_s(buffer, "%4.2f", pl.getEarthSimilarityIndex());
            planetInfo.append("\tESI: ").append(buffer);
            puts(planetInfo.c_str());
            planetInfo.clear();

            sprintf_s(buffer, "%6.3lf", pl.getMass() * SolarMassToEarthMass);
            planetInfo.append("\tMass (E): ").append(buffer);
            puts(planetInfo.c_str());
            planetInfo.clear();

            sprintf_s(buffer, "%4.0f*C - %3.0f%% ocean, %3.0f%% ice", pl.getSurfaceTemperature() + KelvinToCelsius, 100.0f * pl.getHydroPercentage(), 100.0f * pl.getIcePercentage());
            planetInfo.append("\tSurface Temp: ").append(buffer);
            puts(planetInfo.c_str());
            planetInfo.clear();

            sprintf_s(buffer, "%4.1fg", pl.getSurfaceGravity());
            planetInfo.append("\tSurface Gravity: ").append(buffer);
            puts(planetInfo.c_str());
            planetInfo.clear();

            if (pl.getSurfacePressure() > 0.0f)
            {
                sprintf_s(buffer, "%7.3fatm - %3.0f%% cloud coverage", pl.getSurfacePressure() / EarthSurfacePressureMb, 100.0f * pl.getCloudPercentage());
            }
            else
            {
                strcpy_s(buffer, "No atmosphere");
            }
            planetInfo.append("\tSurface Press: ").append(buffer);
            puts(planetInfo.c_str());
            planetInfo.clear();

            if (pl.getEarthSimilarityIndex() > 0.80f)
            {
                assert(pl.getSurfacePressure() > 0.0f);
                puts("\tAtmosphere:");

                bool firstTrace = true;
                for (const auto& gas : pl.getAtmo())
                {
                    bool showPercent = (gas.fraction >= 0.001f);
                    bool showPPM = (gas.fraction >= 0.000001f);

                    if (showPercent)
                    {
                        sprintf_s(buffer, "%5.1f%%", gas.fraction * 100.0f);
                        planetInfo.append("\t\t").append(GasName(gas.gas)).append(": ").append(buffer);
                        puts(planetInfo.c_str());
                        planetInfo.clear();
                    }
                    else if (showPPM)
                    {
                        sprintf_s(buffer, "%3.0fppm", gas.fraction * 1000000.0f);
                        planetInfo.append("\t\t").append(GasName(gas.gas)).append(": ").append(buffer);
                        puts(planetInfo.c_str());
                        planetInfo.clear();
                    }
                    else
                    {
                        if (firstTrace)
                        {
                            firstTrace = false;
                            planetInfo.append("\t\t").append("Trace ");
                        }
                        else
                        {
                            planetInfo.append(", ");
                        }
                        planetInfo.append(GasName(gas.gas));
                    }
                }
                puts(planetInfo.c_str());
                planetInfo.clear();
            }
        }
        else // Is gaseous
        {
            sprintf_s(buffer, "%.3lf", pl.getMass() * SolarMassToJovianMass);
            planetInfo.append("\tMass (Jovian):\t").append(buffer);
            puts(planetInfo.c_str());
            planetInfo.clear();
        }
    }
    else // AsteroidBelt
    {
        planetInfo.append("\tSemi-major axis: ");
        planetInfo.append(SMA(pl.getSemimajorAxis()));
        puts(planetInfo.c_str());
        planetInfo.clear();
    }
}

//----------------------------------------------------------------------------
void ShowResults(const SolarSystem& ss, const Generator& gen, bool showSummary, bool showDetails)
{
    char starClass[4];
    ss.getStar().getStellarClass(starClass, sizeof(starClass));

    if (showSummary)
    {
        const size_t numPlanets = ss.getPlanets().size();

        OrbitalZone lastZone = OrbitalZone::Inner;

        printf("Central Star: %s - seed 0x%I64X\n %u protoplanets consumed\n", starClass, gen.getSeed(), gen.getProtoplanetCount());
        printf("Habitable Zone: %.3lfAU - %.3lfAU\nEcosphere     : %.3lfAU\n\n",
               ss.getStar().getHabitableZone().first, ss.getStar().getHabitableZone().second,
               ss.getStar().getEcosphere());

        printf("=== INNER ZONE     %6.3lfAU ==================================================\n", 0.0);
        int ordinal = 1;
        for (const auto& pl : ss.getPlanets())
        {
            const OrbitalZone thisZone = ss.getStar().getOrbitalZone(pl.getSemimajorAxis());
            if (thisZone != lastZone)
            {
                if (thisZone == OrbitalZone::Habitable)
                {
                    printf("=== HABITABLE ZONE %6.3lfAU ==================================================\n", ss.getStar().getHabitableZone().first);
                }
                else if (thisZone == OrbitalZone::Middle)
                {
                    if (lastZone != OrbitalZone::Habitable)
                    {
                        printf("=== HABITABLE ZONE %6.3lfAU ==================================================\n", ss.getStar().getHabitableZone().first);
                    }
                    printf("=== MIDDLE ZONE    %6.3lfAU ==================================================\n", ss.getStar().getHabitableZone().second);
                }
                else if (thisZone == OrbitalZone::Outer)
                {
                    printf("=== OUTER ZONE     %6.3lfAU ==================================================\n", ss.getStar().getSnowLine());
                }
                lastZone = thisZone;
            }

            std::string planetInfo;
            planetInfo.append(PlanetOrdinal(ordinal++));
            planetInfo.append(2, ' ').append(1, AsciiArtType(pl.getPlanetType())).append(2, ' ');
            planetInfo.append(SMA(pl.getSemimajorAxis()));
            planetInfo.append(Radius(pl.getRadius()));

            if (!pl.isGaseous())
            {
                planetInfo.append(MeanSurfaceConditions(pl.getSurfaceTemperature() + KelvinToCelsius, pl.getSurfacePressure()));
                char esi[16];
                sprintf_s(esi, "esi: %4.2f", pl.getEarthSimilarityIndex());
                planetInfo.append(esi);
            }

            puts(planetInfo.c_str());
            /*if (pl.sizeMoon() > 0)
            {
                std::for_each(pl.beginMoon(), pl.endMoon(), [&](const auto& m)
                              {
                                  std::string moonInfo(MoonOrdinal(pl.ordinal(), m.ordinal()));
                                  moonInfo.append(2, ' ').append(1, AsciiArtType(m.planetType())).append(2, ' ');
                                  moonInfo.append(km(m.sma()));
                                  moonInfo.append(Radius(m.r()));
                                  if (!m.isGaseous())
                                  {
                                      moonInfo.append(MeanSurfaceConditions(m.surfaceTemperature() + KelvinToCelsius, m.surfacePressure()));
                                      char esi[16];
                                      sprintf_s(esi, "esi: %4.2f", m.esi());
                                      moonInfo.append(esi);
                                  }

                                  puts(moonInfo.c_str());
                              });
            }*/
        }
    }

    if (showDetails)
    {
        printf("\nCentral Star: %s\n", starClass);

        int32_t ordinal = 1;
        for (const auto& pl : ss.getPlanets())
        {
            puts("\n==============================================================================");

            SpewPlanet(pl, ordinal++);
            /*if (pl.sizeMoon() > 0)
            {
                std::for_each(pl.beginMoon(), pl.endMoon(), [&](const auto& m)
                              {
                                  puts("");
                                  SpewPlanet(m, pl.ordinal());
                              });
            }*/
        }
    }
}

//----------------------------------------------------------------------------
int main(int, char**)
{
    Generator gen;
    //gen.seed(12345);
    //gen.seed(0x7d9a6763eae90ecbull);

    uint64_t timeSeed = time(nullptr);
    timeSeed = 6364136223846793005ULL * timeSeed + 1ull;
    uint32_t tm = static_cast<uint32_t>(timeSeed >> 32u);
    tm ^= tm >> 11;
    tm ^= tm << 7 & 0x9D2C5680;
    tm ^= tm << 15 & 0xEFC60000;
    tm ^= tm >> 18;
    const uint64_t seed = (timeSeed & 0xffffffffull) | (static_cast<uint64_t>(tm) << 32ull);
    gen.seed(seed);


    Star sun;
    sun.setType(StarClassification::G_V, 2);
    sun.setName("Bob");
    sun.evaluate(&gen);

    SolarSystem ss;
    ss.setName("Bob System");
    ss.add(sun);

    Config cfg;
    cfg.generateStar = true;
    cfg.generateBodeSeeds = true;
    //cfg.verboseLogging = true;
    gen.generate(ss, cfg);

    const bool showSummary = false;
    const bool showDetails = true;

    ShowResults(ss, gen, showSummary, showDetails);

    return 0;
}
