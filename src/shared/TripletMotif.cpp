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

#include "TripletMotif.h"
#include <stdexcept>
#include <math.h>

TripletMotif::TripletMotif(std::list< ConnectionType > connections)
{
	size = 3;
	for(unsigned int ii = 0; ii < size; ++ii)
	{
		for(unsigned int jj = 0; jj < size; ++jj)
		{
			this->connections[ii][jj] = false;
		}
	}
	std::list< ConnectionType >::const_iterator connectionIt;
	for(connectionIt = connections.begin(); connectionIt != connections.end(); ++connectionIt)
	{
		this->connections[connectionIt->first][connectionIt->second] = true;
	}
}


double TripletMotif::computeOccurrenceProbability(std::vector< std::vector< double > > innervation)
{
	if(!innervation.size())
	{
		throw std::out_of_range("Innervation matrix empty");
	}
	else if(innervation.size() != size || innervation[0].size() != size)
	{
		throw std::out_of_range("Motif and innervation matrix dimensions not matching");
	}

	double probability = 1;
	for(unsigned int ii = 0; ii < size; ++ii)
	{
		for(unsigned int jj = 0; jj < size; ++jj)
		{
			if(ii == jj)
			{
				continue;
			}
			double prob = 1 - exp(-1*innervation[ii][jj]);
			if(connections[ii][jj])
			{
				probability *= prob;
			}
			else
			{
				probability *= (1 - prob);
			}
		}
	}
	return probability;
}


double TripletMotif::computeOccurrenceProbabilityGivenInputProbability(std::vector< std::vector< double > > convergence)
{
	if(!convergence.size())
	{
		throw std::out_of_range("Convergence matrix empty");
	}
	else if(convergence.size() != size || convergence[0].size() != size)
	{
		throw std::out_of_range("Motif and convergence matrix dimensions not matching");
	}

	double probability = 1;
	for(unsigned int ii = 0; ii < size; ++ii)
	{
		for(unsigned int jj = 0; jj < size; ++jj)
		{
			if(ii == jj)
			{
				continue;
			}
			double prob = convergence[ii][jj];
			if(connections[ii][jj])
			{
				probability *= prob;
			}
			else
			{
				probability *= (1 - prob);
			}
		}
	}
	return probability;
}

bool TripletMotif::getConnections(unsigned int idx1, unsigned int idx2)
{
	return connections[idx1][idx2];
}

unsigned int TripletMotif::computeOccurrencesWithStrength(std::vector< std::vector< unsigned int > > synapseRangeMatrix)
{
	if(!synapseRangeMatrix.size())
	{
		throw std::out_of_range("Synapse range matrix empty");
	}
	else if(synapseRangeMatrix.size() != size || synapseRangeMatrix[0].size() != size)
	{
		throw std::out_of_range("Motif and synapse range matrix dimensions not matching");
	}

	unsigned int totalNumber = 1;
	for(unsigned int ii = 0; ii < size; ++ii)
	{
		for(unsigned int jj = 0; jj < size; ++jj)
		{
			if(ii == jj)
			{
				continue;
			}
			if(connections[ii][jj])
			{
				totalNumber *= synapseRangeMatrix[ii][jj];
			}
		}
	}
	return totalNumber;
}


CellTriplet::CellTriplet(unsigned int preCell1, unsigned int preCell2, unsigned int preCell3, unsigned int postCell1, unsigned int postCell2, unsigned int postCell3)
{
	preCellIndex.push_back(preCell1);
	preCellIndex.push_back(preCell2);
	preCellIndex.push_back(preCell3);
	postCellIndex.push_back(postCell1);
	postCellIndex.push_back(postCell2);
	postCellIndex.push_back(postCell3);
}

/*
void CellTriplet::setInnervationMatrix(ConnectionMatrix * connectome)
{
	for(int ii = 0; ii < 3; ++ii)
	{
		std::vector< double > emptyRow;
		this->innervation.push_back(emptyRow);
		for(int jj = 0; jj < 3; ++jj)
		{
			if(ii == jj)
			{
				this->innervation[ii].push_back(0);
				continue;
			}

			MatrixIndexType index;

			if (connectome->simpleMatrix)
			{
				std::map<unsigned int, unsigned int>::iterator it = connectome->preDuplicatedIDtoSingleID.find(preCellIndex[ii]);

				if (it != connectome->preDuplicatedIDtoSingleID.end())
				{
					index = std::make_pair((*it).second, postCellIndex[jj]);
				}
				else
				{
					std::cout << "ERROR! PreCellID " << preCellIndex[ii] << " was not found!" << std::endl;
				}
			}
			else
			{
				index = std::make_pair(preCellIndex[ii], postCellIndex[jj]);
			}

			double innervation = 0;
			if(connectome->matrix.find(index) != connectome->matrix.end())
			{
				innervation = connectome->matrix[index];
			}
			this->innervation[ii].push_back(innervation);
		}
	}
}
*/


void CellTriplet::setAverageConvergenceMatrix(std::vector< std::vector< double > > avgConvergence)
{
	this->avgConvergence = avgConvergence;
}
