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

#include <assert.h>

namespace
{

//----------------------------------------------------------------------------
// Make a Roman numeral out of an integer.  Only supports positive values less
// than 100.
std::string Int2Roman(int32_t number)
{
    // For now, assert on it as a reminder that I need to expand the range.
    assert(number < 100);

    std::string r;
    if (number > 0)
    {
        const int32_t tens = number / 10;
        const int32_t units = number - tens * 10;

        switch (tens)
        {
            case 1:
                r.append("X");
                break;
            case 2:
                r.append("XX");
                break;
            case 3:
                r.append("XXX");
                break;
            case 4:
                r.append("XL");
                break;
            case 5:
                r.append("L");
                break;
            case 6:
                r.append("LX");
                break;
            case 7:
                r.append("LXX");
                break;
            case 8:
                r.append("LXXX");
                break;
            case 9:
                r.append("XC");
                break;
            case 0:
            default:
                ;
        }
        switch (units)
        {
            case 1:
                r.append("I");
                break;
            case 2:
                r.append("II");
                break;
            case 3:
                r.append("III");
                break;
            case 4:
                r.append("IV");
                break;
            case 5:
                r.append("V");
                break;
            case 6:
                r.append("VI");
                break;
            case 7:
                r.append("VII");
                break;
            case 8:
                r.append("VIII");
                break;
            case 9:
                r.append("IX");
                break;
            case 0:
            default:
                ;
        }
    }

    return r;
}

}

namespace qc
{

namespace SystemGenerator
{

//----------------------------------------------------------------------------
void SolarSystem::add(const Star& star_)
{
    reset();

    star = star_;
    star.evaluate();

    // If the star isn't named, name it after the system.
    if (star.getName().empty())
    {
        star.setName(name);
    }
}

//----------------------------------------------------------------------------
void SolarSystem::evaluate(Generator& generator)
{
    // The star should have been evaluated when it was added.  If this is the
    // default star, I need to evaluate it now.
    star.evaluate(&generator);

    // If the star isn't named, name it after the system.
    if (star.getName().empty())
    {
        star.setName(name);
    }

    int ordinal = 1;
    for (auto& p : planet)
    {
        if (p.getName().empty())
        {
            // Name anonymous planets [Star Name] + [Ordinal]
            std::string newName = star.getName() + std::string(" ") + Int2Roman(ordinal);
            p.setName(newName);
        }

#if 0
        int moonOrdinal = 1;
        for (auto& m : p.getMoons())
        {
            if (m.getName().empty())
            {
                // Name anonymous moons [Planet Name] + [ordinal as lower-case alphabet]
                std::string newMoonName = p.getName();
                char num[16];
                if (moonOrdinal <= 26)
                {
                    sprintf_s(num, " %c", 'a' + moonOrdinal - 1);
                }
                else
                {
                    sprintf_s(num, " %d", moonOrdinal);
                }
                newMoonName.append(num);

                m.setName(newMoonName);
            }
            ++moonOrdinal;
        }
#endif

        p.evaluate(generator, star);

        ++ordinal;
    }
}

//----------------------------------------------------------------------------
void SolarSystem::reset()
{
    star.reset();
    planet.clear();
}

}
}
