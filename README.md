# qcSystemGenerator
 An updated accretion model solar system generator by MOARdV / Questionable Coding

## INTRODUCTION

This is yet another planetary system generator derived from accrete.  Per the credits and readmes
in the other implementations, accrete originated in the early 1970s, but most reproductions can
trace their origin to Matt Burdick's 1988 (and subsequent) edition.

While I was responsible for one of the Burdick-derived versions in the 1990s, this one is descended
from the Jim Burrows stargen @ http://www.eldacur.com/~brons/

The goal of this version is to provide
an agnostic solar system generator, as opposed to one intended for use with a specific tabletop role-playing game.
It is intended to generate solar systems with Earth-like habitable planets if seeds are not provided.

The code is organized into the qcSystemGenerator static library project plus a one-file example project that demonstrates its use.
It is currently geared towards use with Microsoft Visual Studio 2022, since that's the environment that I work in.

The intent of this library is to generate reasonable solar systems that bias towards containing a
planet in the Life Zone, although it does not require that planet to be entirely Earth-like.

## CHANGES

My changes from Burrows stargen implementation include:

* Use C++.
* Take advantage of STL for data structure management.
* Change the seed mechanic used to guide solar system creation.  Instead of using exisiting solar systems for templates,
  the caller may pass one or more (mass, eccentricity) pairs to the generator to seed preferred planets in the proto-solar system.
* When caller-supplied seeds are not used, the caller has the option of letting the
  system generator create seeds based on a variation of the Titius-Bode Law.
* Use the Kothari radius computation for all planet types, not just rocky planets.  I found that the radius
  computation was better for gas giants than the empirical_density() / volume_radius() approach, based on
  our solar system's gaseous planets.
* Modify the Zone classification used to determine atomic weights and numbers consumed by the Kothari
  equation so that the zone transitions have a width greater than zero.  Instead of a discontinuity at
  each transition, there's a blend between the zones.
* Significantly change the planetary evaluation.  There were a number of redundant evaluations, and
  a number of unused values that were computed that were notionally duplicative of other values.
* Change the planet type enumerants.  There wasn't a lot of explanation of the types, although I could infer their
  intent based on the criteria used to select them.
* Use random numbers a little bit more - such as adding some variation to albedos.
* Document the code.  Documentation was spotty.  I strived to provide more robust explanations for what
  was going on, and why I did it the way I did.  Some of these comments are to highlight where I did things
  differently, and why.

## BUILDING

The project includes a solution file for Microsoft Visual Studio 2022.  I've tried to stick to STL and conventional C++14
constructs, so I think it can be built with other compilers.  I don't have access to an environment where I can
test that, however.  If someone wants to set up cross-compiling support and submit a pull request, I'd appreciate it.

## USAGE

Quickstart:

1) Instantiate a Config structure.

2) (Optional) Override one or more parameters in the Config structure to customize planetary system creation.

3) Call one of the System::create() methods.  When create returns, System::star and System::PlanetList are
   fully evaluated.

## CONTRIBUTING

I welcome contributions.  If someone wants to provide a way to cross-compile without abandoning the MSVS solution/project, I'd be happy
to take a pull request (keeping the solution/project intact is a requirement, since I work exclusively in the MSVS 2022 environment currently).

Please feel free to open issues to discuss changes or bugs, or to submit pull requests if there are errors that need addressed.

## LICENSE

The library and example application are both released under the MIT license.

## REFERENCES

* Blagg 1913: "On a Suggested Substitute for Bode's Law", Mary A. Blagg, Monthly Notices of the Royal Astronomical Society. Vol 73, pp. 414-422, 1913
* Burrows 2006: stargen C-language software source code.  Copy found at https://github.com/zakski/accrete-starform-stargen/
* Burdick 1985: Accrete C-language software source code.  Copy found at https://github.com/zakski/accrete-starform-stargen/tree/master/originals/burdick/accrete
* Chen, et al. 2017: "Probabilistic Forecasting of the Masses and Radii of Other Worlds",  Jingjing Chen and David Kipping, The Astrophysical Journal, Vol 834, 17
* Dole 1969: "Formation of Planetary Systems by Aggregation: A Computer Simulation", S. H. Dole, RAND paper no. P-4226, 1969
* Fogg 1985: "Extra-Solar Planetary Systems: A Microcomputer Simulation", Martyn J. Fogg,  Journal of the British Interplanetary Society, Vol 38, pp. 501-514, 1985
* Kasting 1993: "Habitable Zones around Main Sequence Stars", James F. Kasting, et al., Icarus, Volume 101, Issue 1, January 1993, Pages 108-128
* Kothari 1936: "The Internal Constitution of Planets", Dr. D. S. Kothari,  Mon. Not. of the Royal Astronomical Society, Vol 96, pp. 833-843, 1936
* PHL@UPR Arecibo: "Earth Similarity Index", https://phl.upr.edu/projects/earth-similarity-index-esi
* Pollard 1979: "The prevalence of Earthlike Planets", W. G. Pollard, Amer. Sci., Vol 67, pp. 653-659, 1979
* Zeigler 1999: "GURPS Traveller First In", Jon F. Zeigler, Steve Jackson Games, 1999

## PAST README AND CREDITS

The following is the chain of readmes and credits included with stargen, as found at
https://github.com/zakski/accrete-starform-stargen :

```text
StarGen/starform/accrete has a long and varied history. Many people have altered it over time.
The original program was written in Fortran about 30 years ago at Rand. About 15 years ago, in
1985, Martyn Fogg wrote a version of it and published his results. This lead Matt Burdick to
create a version first in Pascal and then in C. This program was called "starform" that showed
up on the net in about 1988. Several versions of starform have since been developed and
distributed by Ian Burrell, Carl Burke and Chris Croughton AKA Keris.

Each of the authors has attempted to preserve the credits of those who came before, but it is
often hard to know whose voice the different bits are written in. This is my attempt to
continue the tradition of giving Credit where credit is due. I've missed things and made
mistakes, no doubt.

JimB. aka Brons
Jim Burrows
Eldacur Technologies
brons@eldacur.com
http://www.eldacur.com/~brons/

        ================================================================
                Taken from Chris Croughton AKA Keris
        ================================================================

Starform (also known as Accrete) was created by Matt Burdick in 1988.  Since
then it has been hacked about by many people.  This version is based on the
original released 'starform' code, as far as I know, with my hacks to it to
make it do what I want.

My changes are:

  Convert it to use ANSI C function prototypes, and have a header file for
  each module containing the exported functions and variables.

  Make it more modular, so that modules are not using data from modules that
  call them (and hopefully not calling functions 'upward' either).
  
  Put the 'main' function as the only thing in starform.c, so that (hopefully)
  all the other modules can be called by other programs.

  Added stuff to list what materials are liquid on the planets' surface and
  what gasses might be in the atmosphere (if any).  It's guesswork to an
  extent (especially proportions), if you want to take it out look for
  function 'text_list_stuff' in module display.c and delete the calls to it.
  If you know more than I do about the amounts or properties (not hard!)
  please let me know any changes to make it better.

  I've also added some ad-hoc moon generation stuff, if you define 'MOON' when
  compiling (add '-DMOON' to the CFLAGS variable in the makefile) then you'll
  get moons generated as well.  It's not scientifically based, though, so you
  probably don't want to do that...

Please read the INSTALL file to find out how to build it.

Chris Croughton, 1999.04.19
mailto:chris@keris.demon.co.uk

        ================================================================
                Taken from Matt Burdick (via Kerris's ReadMe)
        ================================================================

This program is based on an article by Martyn Fogg in the Journal of the
British Interplanetary Society (JBIS) called 'Extrasolar Planetary Systems:
a Microcomputer Simulation'.  In it, he described how to generate various
sun-like solar systems randomly.  Since he did a good job of listing
references, I decided to implement it in Turbo Pascal on my PC.

Later, I translated it to C for portability, and the result is what you see
in front of you.  Because of my need to run this on an IBM-PC, there are two
makefiles included with the program: 'Makefile' (for unix systems), and 
'starform.mak' for MS-DOS systems.  To create the executable on a unix 
system, type in 'make' alone; type 'make starform.mak' if you are on 
an MS-DOS machine and using Microsoft C.

Thanks go to Sean Malloy (malloy@nprdc.arpa) for his help with the random
number routines and to Marty Shannon (mjs@mozart.att.com) for the
lisp-style output format, command-line flags, and various other pieces.

Enjoy, and if you find any glaring inconsistancies or interesting pieces to
add to the simulation, let me know and I'll include it in any other
distributions I send out.

Now for some references.  These are not the only good references on this 
subject; only the most interesting of many that were listed in Fogg's 
article in vol 38 of JBIS:

For a good description of the entire program:
    "Extra-Solar Planetary Systems: A Microcomputer Simulation"
    Martyn J. Fogg,  Journal of the British Interplanetary Society
    Vol 38, pp. 501 - 514, 1985

For the surface temperature/albedo iterative loop:
    "The Evolution of the Atmosphere of the Earth"
    Michael H. Hart, Icarus, Vol 33, pp. 23 - 39, 1978

For the determination of the radius of a terrestrial planet:
    "The Internal Constitution of the Planets"
    D. S. Kothari, Ph.D. , Mon. Not. Roy. Astr. Soc.
    Vol 96, pp. 833 - 843, 1936

For the planetary mass accretion algorithm:
    "Formation of Planetary Systems by Aggregation: A Computer Simulation"
    S. H. Dole, RAND paper no. P-4226, 1969

For the day length calculation:
    "Q in the Solar System"
    P. Goldreich and S. Soter, Icarus, Vol 5, pp. 375 - 389, 1966

----------------------------------------------------------------------
 I can be reached at the email address burdick%hpda@hplabs.hp.com
----------------------------------------------------------------------

        ================================================================
                From Carl Burke's credits of his version:
        ================================================================

Source Code 
I've contacted Matt Burdick (the author of starform) to find out exactly what
restrictions he put on his code. (To be more precise, his brother saw this page and made the
introductions.) As far as he and I are concerned, this software is free for your use as long
as you don't sell it. I take that to mean that you can include this code in a commercial
package (like a game) without fee, as long as that package isn't just a prettier version of
this applet. There's a longer copyright notice in the source files, but this gets the gist
across.
The source code is in this Unix tarred & gzipped file, or in this Windows/DOS zip archive.

------------------------------------------------------------------------
Acknowledgements 
Matt Burdick, the author of 'starform' (freely redistributable); much of the code
(particularly planetary environments) was adapted from his code.
Andrew Folkins, the author of 'accretion' (public domain) for the Amiga; I used chunks of his
code when creating my displays.
Ed Taychert of Irony Games, for the algorithm he uses to classify terrestrial planets in his
tabular CGI implementation of 'starform'.
Paul Schlyter, who provided information about computing planetary positions.
Planetary images courtesy Jet Propulsion Laboratory. Copyright (c) California Institute of
Technology, Pasadena, CA. All rights reserved. 

------------------------------------------------------------------------
Bibliography 
These sources are the ones quoted by Burdick in the code. A good web search (or more
old-fashioned literature search) will identify literally hundreds of papers regarding the
formation of the solar system and the evolution of proplyds (protoplanetary discs). Most of
these sources can be difficult for a layman like myself to locate, but those journals are the
best place to get up to speed on current theories of formation.

"Extra-Solar Planetary Systems: A Microcomputer Simulation", Martyn J. Fogg, Journal of the
 British Interplanetary Society Vol 38, pp. 501 - 514, 1985

"The Evolution of the Atmosphere of the Earth", Michael H. Hart, Icarus, Vol 33, pp. 23 - 39,
 1978.

"The Internal Constitution of the Planets", D. S. Kothari, Ph.D. , Mon. Not. Roy. Astr. Soc.
 Vol 96, pp. 833 - 843, 1936

"Formation of Planetary Systems by Aggregation: A Computer Simulation", S. H. Dole, RAND paper
 no. P-4226, 1969

"Habitable Planets for Man", S. H. Dole, Blaisdell Publishing Company, NY, 1964.

"Q in the Solar System", P. Goldreich and S. Soter, Icarus, Vol 5, pp. 375 - 389, 1966

        ================================================================
                From Ian Burrell's web page.
        ================================================================

History

Long ago, before I was born, there was published an article by Dole about simulating the
creation of planetary systems by accretion. This simple model was later analyzed in another
paper by Carl Sagan. A later article published Fortran code for implementing the original
algorithm and for simulating the creation of a planetary system. 

As far as I know, the first widespread implementation was done by Matt Burdick. He wrote a
Turbo Pascal version, a C version, and put together a package called starform. All of these
implementations include environmental code to create and calculate temperature, atmosphere,
and other environmental parameters beyond those of the Dole accretion model. 

The accrete program got a wide distribution from the USML mailing list in 1988. That mailing
list's lofty goal was to explore the simulation of the universe, concentrating on the
creation of fictional planetary systems, random terrain generation, and modeling human
societies. It eventually died out due to lack of interest and a broadening of topic. 

Although this model is good enough for creating fictional star systems, the current theory
for the creation of the solar system has moved on. Current scientific models are much more
complicated and require significant computation time. The discovery of extrasolar planets has
raised significant questions about the formation of planets. 

References

*	Dole, S. "Computer Simulation of the Formation of Planetary Systems". Icarus, vol 13,
    pp 494-508, 1970. 
*	Isaacman, R. & Sagan, C. "Computer Simulation of Planetary Accretion Dynamics:
    Sensitivity to Initial Conditions". Icarus, vol 31, p 510, 1977. 
*	Fogg, Martyn J. "Extra-Solar Planetary Systems: A Microcomputer Simulation". Journal of
    the British Interplanetary Society, vol 38, p 501-514, 1985. 

------------------------------------------------------------------------
Ian Burrell / iburrell@znark.com

        ================================================================
                                  Done
        ================================================================
```
