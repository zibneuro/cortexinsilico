/****************************************************************************/
/* Code adapted from:                                                       */
/*                                                                          */
/* Author:    Robert Egger                                                  */
/*            Max Planck Institute for Biological Cybernetics               */
/*            Spemannstr. 38-44                                             */
/*            72076 Tuebingen                                               */
/*            Germany                                                       */
/*                                                                          */
/* EMail:     robert.egger@tuebingen.mpg.de                                 */
/*                                                                          */
/* History:   17.06.2014                                                    */
/*                                                                          */
/* Remarks:   All rights are reserved by the Max-Planck-Society             */
/*                                                                          */
/****************************************************************************/

#ifndef MOTIFCOMBINATIONS_H
#define MOTIFCOMBINATIONS_H

#include "TripletMotif.h"
#include <map>
#include <list>

/**
    This class generates all combinatorially possible motif configurations.
*/
class MotifCombinations {
public:

/**
    Generates mapping of the 16 canonical triplet motifs to the 64
    combinatorially possible connection configurations between three neurons.
    @returns The mapping of the 16 non redundant motifs to the 64 triplet combinations.
*/
std::map< unsigned int, std::list< TripletMotif* > > initializeNonRedundantTripletMotifs();

};

#endif // MOTIFCOMBINATIONS_H
