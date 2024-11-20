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
#pragma once

#include "Consts.h"

#include <algorithm>

namespace qc
{

namespace SystemGenerator
{

//----------------------------------------------------------------------------

/// @brief Clamp a value between upper and lower bounds.
/// @tparam T_ Value type
/// @param value The value to clamp
/// @param lower The lower bound of the range.
/// @param upper The upper bound of the range.
/// @return The clamped value.
template <class T_> inline T_ Clamp(T_ value, T_ lower, T_ upper)
{
    return std::min(upper, std::max(value, lower));
}

//----------------------------------------------------------------------------

/// @brief Given the semi-major axis and eccentricity of an orbit, and the luminosity
/// of the star, returns the minimum mass required to retain gases.
/// 
/// Protoplanets that exceed this mass will retain gas and become gas giants.
/// @param sma The SMA, in AU.
/// @param eccentricity Eccentricity of the orbit, [0.0, 1.0).
/// @param stellarLuminosity Luminosity of the star, Sol = 1.0.
/// @return Minimum planetoid mass for gas retention, in Solar masses.
double CriticalLimit(double sma, double eccentricity, double stellarLuminosity);

//----------------------------------------------------------------------------

/// @brief Returns the escape velocity of a body
/// @param mass Mass of the body, in Solar masses.
/// @param radius Radius of the body, in km.
/// @return Escape velocity, in m/s.
inline double EscapeVelocity(double mass, double radius)
{
    return MPerCm * sqrt(2.0 * GravityConstant * mass * SolarMassInGrams / (radius * CmPerKm));
}

//----------------------------------------------------------------------------

/// @brief Return the interpolant required to return `value` when lerping between
/// `lower` and `upper`.
/// @tparam T_ Value type
/// @param value The value to inverse lerp.
/// @param lower The lower range of values.
/// @param upper The upper range of values.
/// @return The interpolant in the range [0.0, 1.0].
template <class T_> inline T_ InverseLerp(T_ value, T_ lower, T_ upper)
{
    return (value <= lower) ? (T_)0 : ((value >= upper) ? (T_)1 : ((value - lower) / (upper - lower)));
}

//----------------------------------------------------------------------------

/// @brief Compute the radius of a planet, given the planet's sma and mass, the Star,
/// and whether the planet should be treated as a gas giant.
/// 
/// In Fogg 1985, this function is Eq. 9.  It originated as Eq. 23 in Kothari 1936.
/// The accrete implementation, going back to Burdick 1988, did not use the Kothari equation
/// for gas giants.  Fogg 1985 says that a different algorithm did a good enough
/// job in matching rocky planet's radii.  However, Burdick 1988, et seq, use Kothari's
/// equation for rocky planets and empirical_density for gas giants.  It was not clear
/// to me why it was implemented like that at first.
///
/// The Kothari equation does a better job than empirical_density() for some gas giants and
/// rocky planets, so I initially used this function exclusively, with the
/// following changes (following the background information).
/// 
/// Fogg 1985 describes three zones of dust, Zones 1, 2, and 3.  The zones originated in
/// Pollard 1979.  Zone 1 contains Class III material; Zone 2 contains Class I, Class II,
/// and Class III material; and Zone 3 contains Class II and Class III material.
/// 
/// Class I material is H2 and He.  In Zone 1, it's been swept up by the star, I think.
/// Or else it's too warm, so it's been ejected farther out in the dust cloud.
/// In Zone 3, the gravitational force of the star is too weak to retain the material, so
/// it has escaped.
/// 
/// Class II material has volatile ices containing CHO.  Zone 1 is too warm for it, but it is
/// available in Zone 2 and Zone 3.
/// 
/// Class III material is heavier matter available throughout the dust cloud.
/// 
/// I don't have Pollard 1979, so I am making some guesses below that may be invalid.  If I
/// can find a discussion of it, I'll refine the below.
/// 
/// The Zone 1 to Zone 2 transition is around 4 AU in our solar system.  Since Zeigler 1999
/// uses 5 AU as the demarcation of the Snow Line, I am making the Z1-Z2 transition 1 AU wide
/// (Z2 begins at 4 AU, Z1 ends at 5 AU).
/// 
/// Pollard 1979 says the Zone 2 to Zone 3 transition is around 14 AU to 16 AU, and the Burdick 1985
/// source code uses 15 AU.  I am making this transition 2 AU wide (Z3 begins at 14 AU, Z2 ends at
/// 16 AU).
/// 
/// Within the transition zones, I am applying a linear interpolation between the atomic weights and
/// atomic numbers of the two zones, so there is not a discontinuity in those values when transitioning
/// between zones.  This is purely a speculative change, and I don't have proofs that it is valid.
/// 
/// @note Upon further review of the results, Kothari Radius breaks down under a couple of circumstances:
/// First, in Zone 1, the gas giants are too dense (I've seen density in excess of 10g/cc).  I changed
/// the Z1 densities for gas giants to match Z2, even though H2 and He maybe aren't supposed to be available.
/// Seecond, super-Jovian giants appear to exceed the threshold Kothari identified where the GG starts
/// collapsing due to mass, leading to high-density gas giants (too small to be brown dwarves).
/// 
/// @todo Can the forGasGiant parameter be eliminated?  I already know everything I need, and I can infer
/// if the world is above the critical limit.
/// 
/// @todo Apply some sort of randomization to the weight/mass so that they're not 100% identical?  Maybe
/// a Gaussian distribution with a fairly tight standard deviation?
/// 
/// @param mass Mass of the body, in solar masses.
/// @param sma The semi-major axis of the planet's orbit.  For a moon, the SMA of the parent planet.
/// @param forGasGiant If true, the planet is treated as a gas giant.  If false, it's treated as a rocky planet.
/// @param materialZone The material zone of the star that this planet orbits (I, II, or III, with fractions indicating being in the overlap regions).
/// @return The computed radius, in km.
double KothariRadius(double mass, double sma, bool forGasGiant, float materialZone);

//----------------------------------------------------------------------------

/// @brief Perform a linear interpolation between `lower` and `upper`.
/// @tparam T_ Value type
/// @param interpolant An interpolant in the range [0.0, 1.0].  Values are clamped to that range.
/// @param lower The lower range of values.
/// @param upper The upper range of values.
/// @return Value in the range [lower, upper].
template <class T_> inline T_ Lerp(T_ interpolant, T_ lower, T_ upper)
{
    return (interpolant <= (T_)(0)) ? lower : ((interpolant >= (T_)(1)) ? upper : (lower + (upper - lower) * interpolant));
}

//----------------------------------------------------------------------------

/// @brief Returns the luminosity of a star given `stellarMass`.
/// @todo What is the valid range of inputs?
/// @param stellarMass Mass of the star, with Sol = 1.0.
/// @return Luminosity of the star, with Sol = 1.0.
double Luminosity(double stellarMass);

//----------------------------------------------------------------------------

/// @brief Returns a scalar representing the orbital dominance of a body orbiting
/// a star.
/// 
/// Based on the value PI from Jean-Luc Margot.  A value > 1.0 indicates that
/// the body is massive enough to clear its orbital neighborhood such that the
/// only other bodies in that neighborhood are the body's satellites or other
/// masses that are influenced by the body's gravitational pull.
/// 
/// For instance, Earth has a value of about 810.  Jupiter has about 4000.
/// The asteroid Ceres has about 0.04.
/// @param mass Mass of the body, in Solar masses.
/// @param sma The semi-major axis of the orbit, in AU.
/// @return A numerical value indicating the magnitude of the body's dominance.
inline float OrbitalDominance(double mass, double sma)
{
    // K from the equation to compute PI.
    static constexpr double K = 807.0;

    return static_cast<float>(K * mass * SolarMassToEarthMass * pow(sma, -9.0 / 8.0));
}

//----------------------------------------------------------------------------

/// @brief Returns the orbital period of two bodies, in Earth days.
/// @param distance Distance between the bodies, in AU.
/// @param mass1 Mass of one body, in Solar masses.
/// @param mass2 Mass of the other body, in Solar masses.
/// @return Orbital period, in days.
double Period(double distance, double mass1, double mass2);

//----------------------------------------------------------------------------
/// @brief Returns the root-mean-squared velocity of a given molecule.
/// @param molecularWeight Molecular weight to compute.
/// @param exosphereTemperature Exosphere temperature, in Kelvin.
/// @return RMS velocity, in m/s.
inline double RMSVelocity(double molecularWeight, double exosphereTemperature)
{
    return sqrt(3.0 * MolarGasConstant * exosphereTemperature / molecularWeight);
}

//----------------------------------------------------------------------------

/// @brief Return the density of a body, given its mass and radius.
/// @param mass Mass of the body, in Solar masses.
/// @param radius Radius of the body, in km.
/// @return Density of the body, in g/cc.
double VolumeDensity(double mass, double radius);

}
}
