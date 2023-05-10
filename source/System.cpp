/*****************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2021-2023 Questionable Coding
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
#include "System.h"

#include "GenerationState.h"

#include <time.h>

//----------------------------------------------------------------------------
System::System() :
    protoplanetsGenerated(0),
    rngSeed(0)
{

}

//----------------------------------------------------------------------------
void System::create(const Config* appConfig)
{
    std::vector<PlanetisimalSeed> planetSeed;
    create(appConfig, planetSeed);
}

//----------------------------------------------------------------------------
void System::create(const Config* appConfig, const std::vector<PlanetisimalSeed>& planetSeed)
{
    planetList.clear();
    
    protoplanetsGenerated = 0;

    if (appConfig != nullptr)
    {
        memcpy(&config, appConfig, sizeof(config));
    }
    else
    {
        config = Config();
    }

    if (config.seed == 0)
    {
        uint64_t seed = time(nullptr);
        seed = 6364136223846793005ULL * seed + 1ull;
        uint32_t tm = static_cast<uint32_t>(seed >> 32u);
        tm ^= tm >> 11;
        tm ^= tm << 7 & 0x9D2C5680;
        tm ^= tm << 15 & 0xEFC60000;
        tm ^= tm >> 18;
        config.seed = seed & 0xffffffffull | (static_cast<uint64_t>(tm) << 32ull);
    }

    rngSeed = config.seed;

    if (config.stellarMass <= 0.0f)
    {
        std::mt19937 mt;
        mt.seed(static_cast<uint32_t>(rngSeed & 0xffffffff));
        std::uniform_real_distribution<float> u(MinRandomSolarMass, MaxRandomSolarMass);
        config.stellarMass = u(mt);
    }

    config.stellarMass = std::max(MinimumStellarMass, std::min(MaximumStellarMass, config.stellarMass));

    config.cloudEccentricity = std::max(0.0, std::min(config.cloudEccentricity, 0.99));

    GenerationState state(&config);

    centralStar.init(&config, &state);

    state.finalize(&centralStar);

    std::vector<PlanetisimalSeed> bodeSeed;
    if (planetSeed.empty() && config.generateBodeSeeds)
    {
        // The equation Mary Blagg proposed is 
        // Distance = A * (1.7275 ^ n) * (B + f(a + nb))
        // 
        // Where f(p) = cos(p)/(3 - cos(2p)) + 1 / (6 - 4 * cos(2 * (p - PI/6))
        //
        // In the original paper, A = 0.4162, B = 2.025, a = -0.8*, and b = 56.6*
        // Earth is n = 0, Venus is n = -1, Mars is n = +1 (ordinal values, in other
        // words).
        //
        // I was unable to get the 2nd term to work (B + f(a + nb)) with her
        // numbers - the values were always a little bit off.  I assume I implemented
        // f(p) incorrectly, but it's not that complicated of a function.  So,
        // instead I substitute a Gaussian random distribution.

        // Determine A: A number around 0.41 to 0.46
        const float A = state.randomUniform(0.41f, 0.46f);
        // Determine B: A number such that A * B = centralStar.ecosphere()
        const float B = static_cast<float>(centralStar.ecosphere() / A);

        const float intervalStdDev = B * 0.1f;

        // Place the ideal habitable world position.
        bodeSeed.emplace_back(PlanetisimalSeed(A * state.randomNear(B, intervalStdDev), state.randomEccentricity()));
        ++protoplanetsGenerated;

        // Iterate closer to the star.
        const float innerLimit = static_cast<float>(centralStar.innerPlanetaryLimit());
        float f = -1.0f;
        float sma = A * powf(BodeProgression, f) * state.randomNear(B, intervalStdDev);
        while (sma >= innerLimit)
        {
            bodeSeed.emplace_back(PlanetisimalSeed(sma, state.randomEccentricity()));

            f -= 1.0f;
            sma = A * powf(BodeProgression, f) * state.randomNear(B, intervalStdDev);
            ++protoplanetsGenerated;
        }

        // Iterate away from the star
        const float outerLimit = static_cast<float>(centralStar.outerPlanetaryLimit());
        f = 1.0f;
        sma = A * powf(BodeProgression, f) * state.randomNear(B, intervalStdDev);
        while (sma <= outerLimit)
        {
            bodeSeed.emplace_back(PlanetisimalSeed(sma, state.randomEccentricity()));

            f += 1.0f;
            sma = A * powf(BodeProgression, f) * state.randomNear(B, intervalStdDev);
            ++protoplanetsGenerated;
        }

        // Randomize the order of the seeds
        for (size_t i = 0; i < bodeSeed.size(); ++i)
        {
            std::swap(bodeSeed[i], bodeSeed[state.randomInt<size_t>(1u, bodeSeed.size() - 1u)]);
        }
    }

    distributePlanetaryMass(state, (bodeSeed.empty()) ? planetSeed : bodeSeed);

    evaluatePlanets(&state);
}

//----------------------------------------------------------------------------
void System::distributePlanetaryMass(GenerationState& state, const std::vector<PlanetisimalSeed>& planetSeed)
{
    auto seed = planetSeed.begin();

    while (state.dustAvailable())
    {
        Protoplanet protoplanet;

        // TODO: update innerPlanetaryLimit and outerPlanetaryLimit based on available dust?
        if (seed != planetSeed.end())
        {
            protoplanet.sma = seed->first;
            if (protoplanet.sma < centralStar.innerPlanetaryLimit() || protoplanet.sma > centralStar.outerPlanetaryLimit())
            {
                // Set to an illegal value, so that we don't process this seed.
                protoplanet.sma = -1.0;
                protoplanet.eccentricity = 0.0;
            }
            else
            {
                protoplanet.eccentricity = seed->second;
                if (protoplanet.eccentricity >= 1.0 || protoplanet.eccentricity < 0.0)
                {
                    protoplanet.eccentricity = state.randomEccentricity();
                }
            }
            ++seed;
        }
        else
        {
            protoplanet.sma = state.randomUniform(centralStar.innerPlanetaryLimit(), centralStar.outerPlanetaryLimit());
            protoplanet.eccentricity = state.randomEccentricity();
        }

        // Mass of the protoplanet, in solar masses.  We assume it's all dust, since protoplanetSeedMass should be below
        // the critical limit.  gasMass was zero-initialized.
        protoplanet.dustMass = protoplanet.mass = config.protoplanetSeedMass;

        if (state.dustAvailable(protoplanet))
        {
            state.accreteDust(protoplanet);
            ++protoplanetsGenerated;
        }
    }

    planetList = state.movePlanets();
}

//----------------------------------------------------------------------------
void System::evaluatePlanets(GenerationState* state)
{
    auto planet = planetList.begin();
    int number = 1;
    while (planet != planetList.end())
    {
        planet->evaluate(&centralStar, nullptr, number, &config, state);

        ++planet;
        ++number;
    }
}
