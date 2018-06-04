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

#ifndef TRIPLETMOTIF_H
#define TRIPLETMOTIF_H

#include <vector>
#include <list>

typedef std::pair< unsigned int, unsigned int > ConnectionType;

/**
    This class represents a triplet motif of 3 neurons and the 6 mutual
    connections. Computes the occurrence probability for given innervation
    values.
*/
class TripletMotif
{

public:
	TripletMotif(std::list< ConnectionType > connections);

	double computeOccurrenceProbability(std::vector< std::vector< double > > innervation);
	double computeOccurrenceProbabilityGivenInputProbability(std::vector< std::vector< double > > convergence);
	unsigned int computeOccurrencesWithStrength(std::vector< std::vector< unsigned int > > synapseRangeMatrix);
	bool getConnections(unsigned int idx1, unsigned int idx2);

private:
	bool connections[3][3];
	unsigned int size;
};



class CellTriplet
{
public:
	CellTriplet(unsigned int preCell1, unsigned int preCell2, unsigned int preCell3, unsigned int postCell1, unsigned int postCell2, unsigned int postCell3);

	std::vector< unsigned int > preCellIndex;
	std::vector< unsigned int > postCellIndex;
	std::vector< std::vector< double > > innervation;
	std::vector< std::vector< double > > avgConvergence;

	//void setInnervationMatrix(ConnectionMatrix * connectome);
	void setAverageConvergenceMatrix(std::vector< std::vector< double > > avgConvergence);
};

#endif // TRILETMOTIF_H
