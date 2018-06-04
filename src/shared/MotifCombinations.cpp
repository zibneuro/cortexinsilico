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

#include "MotifCombinations.h"
#include <map>
#include <list>

/**
    Generates mapping of the 16 canonical triplet motifs to the 64
    combinatorially possible connection configurations between three neurons.
    @returns The mapping of the 16 non redundant motifs to the 64 triplet combinations.
*/
std::map< unsigned int, std::list< TripletMotif* > > MotifCombinations::initializeNonRedundantTripletMotifs()
{
	std::list< std::list< ConnectionType > >::const_iterator connectionIt;

	// motifs for network analysis: three edges
	std::list< std::list< ConnectionType > > recurrentLoopConnections;
	std::list< std::list< ConnectionType > > directedLoopConnections;
	std::list< std::list< ConnectionType > > recurrentIncompleteLoopConnections;
	std::list< std::list< ConnectionType > > directedRecurrentLoopConnections;
	std::list< std::list< ConnectionType > > recurrentFeedForwardConvergentConnections;
	std::list< std::list< ConnectionType > > recurrentFeedForwardDivergentConnections;
	std::list< std::list< ConnectionType > > feedForwardConnections;
	std::list< TripletMotif * > recurrentLoopMotifs;
	std::list< TripletMotif * > directedLoopMotifs;
	std::list< TripletMotif * > recurrentIncompleteLoopMotifs;
	std::list< TripletMotif * > directedRecurrentLoopMotifs;
	std::list< TripletMotif * > recurrentFeedForwardConvergentMotifs;
	std::list< TripletMotif * > recurrentFeedForwardDivergentMotifs;
	std::list< TripletMotif * > feedForwardMotifs;

	std::list< ConnectionType > recurrentLoopConnections1;
	recurrentLoopConnections1.push_back(ConnectionType(0,1));
	recurrentLoopConnections1.push_back(ConnectionType(1,0));
	recurrentLoopConnections1.push_back(ConnectionType(1,2));
	recurrentLoopConnections1.push_back(ConnectionType(2,1));
	recurrentLoopConnections1.push_back(ConnectionType(0,2));
	recurrentLoopConnections1.push_back(ConnectionType(2,0));
	recurrentLoopConnections.push_back(recurrentLoopConnections1);
	std::list< ConnectionType > directedLoopConnections1;
	directedLoopConnections1.push_back(ConnectionType(0,1));
	directedLoopConnections1.push_back(ConnectionType(1,2));
	directedLoopConnections1.push_back(ConnectionType(2,0));
	std::list< ConnectionType > directedLoopConnections2;
	directedLoopConnections2.push_back(ConnectionType(1,0));
	directedLoopConnections2.push_back(ConnectionType(0,2));
	directedLoopConnections2.push_back(ConnectionType(2,1));
	directedLoopConnections.push_back(directedLoopConnections1);
	directedLoopConnections.push_back(directedLoopConnections2);
	std::list< ConnectionType > recurrentIncompleteLoopConnection1;
	recurrentIncompleteLoopConnection1.push_back(ConnectionType(0,1));
	recurrentIncompleteLoopConnection1.push_back(ConnectionType(1,2));
	recurrentIncompleteLoopConnection1.push_back(ConnectionType(2,1));
	recurrentIncompleteLoopConnection1.push_back(ConnectionType(0,2));
	recurrentIncompleteLoopConnection1.push_back(ConnectionType(2,0));
	std::list< ConnectionType > recurrentIncompleteLoopConnection2;
	recurrentIncompleteLoopConnection2.push_back(ConnectionType(1,0));
	recurrentIncompleteLoopConnection2.push_back(ConnectionType(1,2));
	recurrentIncompleteLoopConnection2.push_back(ConnectionType(2,1));
	recurrentIncompleteLoopConnection2.push_back(ConnectionType(0,2));
	recurrentIncompleteLoopConnection2.push_back(ConnectionType(2,0));
	std::list< ConnectionType > recurrentIncompleteLoopConnection3;
	recurrentIncompleteLoopConnection3.push_back(ConnectionType(0,1));
	recurrentIncompleteLoopConnection3.push_back(ConnectionType(1,0));
	recurrentIncompleteLoopConnection3.push_back(ConnectionType(1,2));
	recurrentIncompleteLoopConnection3.push_back(ConnectionType(0,2));
	recurrentIncompleteLoopConnection3.push_back(ConnectionType(2,0));
	std::list< ConnectionType > recurrentIncompleteLoopConnection4;
	recurrentIncompleteLoopConnection4.push_back(ConnectionType(0,1));
	recurrentIncompleteLoopConnection4.push_back(ConnectionType(1,0));
	recurrentIncompleteLoopConnection4.push_back(ConnectionType(2,1));
	recurrentIncompleteLoopConnection4.push_back(ConnectionType(0,2));
	recurrentIncompleteLoopConnection4.push_back(ConnectionType(2,0));
	std::list< ConnectionType > recurrentIncompleteLoopConnection5;
	recurrentIncompleteLoopConnection5.push_back(ConnectionType(0,1));
	recurrentIncompleteLoopConnection5.push_back(ConnectionType(1,0));
	recurrentIncompleteLoopConnection5.push_back(ConnectionType(1,2));
	recurrentIncompleteLoopConnection5.push_back(ConnectionType(2,1));
	recurrentIncompleteLoopConnection5.push_back(ConnectionType(2,0));
	std::list< ConnectionType > recurrentIncompleteLoopConnection6;
	recurrentIncompleteLoopConnection6.push_back(ConnectionType(0,1));
	recurrentIncompleteLoopConnection6.push_back(ConnectionType(1,0));
	recurrentIncompleteLoopConnection6.push_back(ConnectionType(1,2));
	recurrentIncompleteLoopConnection6.push_back(ConnectionType(2,1));
	recurrentIncompleteLoopConnection6.push_back(ConnectionType(0,2));
	recurrentIncompleteLoopConnections.push_back(recurrentIncompleteLoopConnection1);
	recurrentIncompleteLoopConnections.push_back(recurrentIncompleteLoopConnection2);
	recurrentIncompleteLoopConnections.push_back(recurrentIncompleteLoopConnection3);
	recurrentIncompleteLoopConnections.push_back(recurrentIncompleteLoopConnection4);
	recurrentIncompleteLoopConnections.push_back(recurrentIncompleteLoopConnection5);
	recurrentIncompleteLoopConnections.push_back(recurrentIncompleteLoopConnection6);
	std::list< ConnectionType > directedRecurrentLoopConnections1;
	directedRecurrentLoopConnections1.push_back(ConnectionType(0,1));
	directedRecurrentLoopConnections1.push_back(ConnectionType(1,2));
	directedRecurrentLoopConnections1.push_back(ConnectionType(0,2));
	directedRecurrentLoopConnections1.push_back(ConnectionType(2,0));
	std::list< ConnectionType > directedRecurrentLoopConnections2;
	directedRecurrentLoopConnections2.push_back(ConnectionType(1,0));
	directedRecurrentLoopConnections2.push_back(ConnectionType(2,1));
	directedRecurrentLoopConnections2.push_back(ConnectionType(0,2));
	directedRecurrentLoopConnections2.push_back(ConnectionType(2,0));
	std::list< ConnectionType > directedRecurrentLoopConnections3;
	directedRecurrentLoopConnections3.push_back(ConnectionType(0,1));
	directedRecurrentLoopConnections3.push_back(ConnectionType(1,0));
	directedRecurrentLoopConnections3.push_back(ConnectionType(1,2));
	directedRecurrentLoopConnections3.push_back(ConnectionType(2,0));
	std::list< ConnectionType > directedRecurrentLoopConnections4;
	directedRecurrentLoopConnections4.push_back(ConnectionType(0,1));
	directedRecurrentLoopConnections4.push_back(ConnectionType(1,0));
	directedRecurrentLoopConnections4.push_back(ConnectionType(2,1));
	directedRecurrentLoopConnections4.push_back(ConnectionType(0,2));
	std::list< ConnectionType > directedRecurrentLoopConnections5;
	directedRecurrentLoopConnections5.push_back(ConnectionType(0,1));
	directedRecurrentLoopConnections5.push_back(ConnectionType(1,2));
	directedRecurrentLoopConnections5.push_back(ConnectionType(2,1));
	directedRecurrentLoopConnections5.push_back(ConnectionType(2,0));
	std::list< ConnectionType > directedRecurrentLoopConnections6;
	directedRecurrentLoopConnections6.push_back(ConnectionType(1,0));
	directedRecurrentLoopConnections6.push_back(ConnectionType(1,2));
	directedRecurrentLoopConnections6.push_back(ConnectionType(2,1));
	directedRecurrentLoopConnections6.push_back(ConnectionType(0,2));
	directedRecurrentLoopConnections.push_back(directedRecurrentLoopConnections1);
	directedRecurrentLoopConnections.push_back(directedRecurrentLoopConnections2);
	directedRecurrentLoopConnections.push_back(directedRecurrentLoopConnections3);
	directedRecurrentLoopConnections.push_back(directedRecurrentLoopConnections4);
	directedRecurrentLoopConnections.push_back(directedRecurrentLoopConnections5);
	directedRecurrentLoopConnections.push_back(directedRecurrentLoopConnections6);
	std::list< ConnectionType > recurrentFeedForwardConvergentConnections1;
	recurrentFeedForwardConvergentConnections1.push_back(ConnectionType(1,0));
	recurrentFeedForwardConvergentConnections1.push_back(ConnectionType(1,2));
	recurrentFeedForwardConvergentConnections1.push_back(ConnectionType(2,1));
	recurrentFeedForwardConvergentConnections1.push_back(ConnectionType(2,0));
	std::list< ConnectionType > recurrentFeedForwardConvergentConnections2;
	recurrentFeedForwardConvergentConnections2.push_back(ConnectionType(0,1));
	recurrentFeedForwardConvergentConnections2.push_back(ConnectionType(2,1));
	recurrentFeedForwardConvergentConnections2.push_back(ConnectionType(0,2));
	recurrentFeedForwardConvergentConnections2.push_back(ConnectionType(2,0));
	std::list< ConnectionType > recurrentFeedForwardConvergentConnections3;
	recurrentFeedForwardConvergentConnections3.push_back(ConnectionType(0,2));
	recurrentFeedForwardConvergentConnections3.push_back(ConnectionType(1,2));
	recurrentFeedForwardConvergentConnections3.push_back(ConnectionType(0,1));
	recurrentFeedForwardConvergentConnections3.push_back(ConnectionType(1,0));
	recurrentFeedForwardConvergentConnections.push_back(recurrentFeedForwardConvergentConnections1);
	recurrentFeedForwardConvergentConnections.push_back(recurrentFeedForwardConvergentConnections2);
	recurrentFeedForwardConvergentConnections.push_back(recurrentFeedForwardConvergentConnections3);
	std::list< ConnectionType > recurrentFeedForwardDivergentConnections1;
	recurrentFeedForwardDivergentConnections1.push_back(ConnectionType(0,1));
	recurrentFeedForwardDivergentConnections1.push_back(ConnectionType(0,2));
	recurrentFeedForwardDivergentConnections1.push_back(ConnectionType(1,2));
	recurrentFeedForwardDivergentConnections1.push_back(ConnectionType(2,1));
	std::list< ConnectionType > recurrentFeedForwardDivergentConnections2;
	recurrentFeedForwardDivergentConnections2.push_back(ConnectionType(1,0));
	recurrentFeedForwardDivergentConnections2.push_back(ConnectionType(1,2));
	recurrentFeedForwardDivergentConnections2.push_back(ConnectionType(0,2));
	recurrentFeedForwardDivergentConnections2.push_back(ConnectionType(2,0));
	std::list< ConnectionType > recurrentFeedForwardDivergentConnections3;
	recurrentFeedForwardDivergentConnections3.push_back(ConnectionType(2,0));
	recurrentFeedForwardDivergentConnections3.push_back(ConnectionType(2,1));
	recurrentFeedForwardDivergentConnections3.push_back(ConnectionType(0,1));
	recurrentFeedForwardDivergentConnections3.push_back(ConnectionType(1,0));
	recurrentFeedForwardDivergentConnections.push_back(recurrentFeedForwardDivergentConnections1);
	recurrentFeedForwardDivergentConnections.push_back(recurrentFeedForwardDivergentConnections2);
	recurrentFeedForwardDivergentConnections.push_back(recurrentFeedForwardDivergentConnections3);
	std::list< ConnectionType > feedForwardConnections1;
	feedForwardConnections1.push_back(ConnectionType(0,1));
	feedForwardConnections1.push_back(ConnectionType(0,2));
	feedForwardConnections1.push_back(ConnectionType(1,2));
	std::list< ConnectionType > feedForwardConnections2;
	feedForwardConnections2.push_back(ConnectionType(0,1));
	feedForwardConnections2.push_back(ConnectionType(0,2));
	feedForwardConnections2.push_back(ConnectionType(2,1));
	std::list< ConnectionType > feedForwardConnections3;
	feedForwardConnections3.push_back(ConnectionType(1,0));
	feedForwardConnections3.push_back(ConnectionType(1,2));
	feedForwardConnections3.push_back(ConnectionType(0,2));
	std::list< ConnectionType > feedForwardConnections4;
	feedForwardConnections4.push_back(ConnectionType(1,0));
	feedForwardConnections4.push_back(ConnectionType(1,2));
	feedForwardConnections4.push_back(ConnectionType(2,0));
	std::list< ConnectionType > feedForwardConnections5;
	feedForwardConnections5.push_back(ConnectionType(2,0));
	feedForwardConnections5.push_back(ConnectionType(2,1));
	feedForwardConnections5.push_back(ConnectionType(0,1));
	std::list< ConnectionType > feedForwardConnections6;
	feedForwardConnections6.push_back(ConnectionType(2,0));
	feedForwardConnections6.push_back(ConnectionType(2,1));
	feedForwardConnections6.push_back(ConnectionType(1,0));
	feedForwardConnections.push_back(feedForwardConnections1);
	feedForwardConnections.push_back(feedForwardConnections2);
	feedForwardConnections.push_back(feedForwardConnections3);
	feedForwardConnections.push_back(feedForwardConnections4);
	feedForwardConnections.push_back(feedForwardConnections5);
	feedForwardConnections.push_back(feedForwardConnections6);

	for(connectionIt = recurrentLoopConnections.begin(); connectionIt != recurrentLoopConnections.end(); ++connectionIt)
	{
		TripletMotif * newMotif = new TripletMotif(*connectionIt);
		recurrentLoopMotifs.push_back(newMotif);
	}
	for(connectionIt = directedLoopConnections.begin(); connectionIt != directedLoopConnections.end(); ++connectionIt)
	{
		TripletMotif * newMotif = new TripletMotif(*connectionIt);
		directedLoopMotifs.push_back(newMotif);
	}
	for(connectionIt = recurrentIncompleteLoopConnections.begin(); connectionIt != recurrentIncompleteLoopConnections.end(); ++connectionIt)
	{
		TripletMotif * newMotif = new TripletMotif(*connectionIt);
		recurrentIncompleteLoopMotifs.push_back(newMotif);
	}
	for(connectionIt = directedRecurrentLoopConnections.begin(); connectionIt != directedRecurrentLoopConnections.end(); ++connectionIt)
	{
		TripletMotif * newMotif = new TripletMotif(*connectionIt);
		directedRecurrentLoopMotifs.push_back(newMotif);
	}
	for(connectionIt = recurrentFeedForwardConvergentConnections.begin(); connectionIt != recurrentFeedForwardConvergentConnections.end(); ++connectionIt)
	{
		TripletMotif * newMotif = new TripletMotif(*connectionIt);
		recurrentFeedForwardConvergentMotifs.push_back(newMotif);
	}
	for(connectionIt = recurrentFeedForwardDivergentConnections.begin(); connectionIt != recurrentFeedForwardDivergentConnections.end(); ++connectionIt)
	{
		TripletMotif * newMotif = new TripletMotif(*connectionIt);
		recurrentFeedForwardDivergentMotifs.push_back(newMotif);
	}
	for(connectionIt = feedForwardConnections.begin(); connectionIt != feedForwardConnections.end(); ++connectionIt)
	{
		TripletMotif * newMotif = new TripletMotif(*connectionIt);
		feedForwardMotifs.push_back(newMotif);
	}

	// motifs for network analysis: two edges
	std::list< std::list< ConnectionType > > recurrentIncompleteConnections;
	std::list< std::list< ConnectionType > > feedForwardIncompleteConnections;
	std::list< std::list< ConnectionType > > recurrentDivergentConnections;
	std::list< std::list< ConnectionType > > recurrentConvergentConnections;
	std::list< std::list< ConnectionType > > feedForwardConvergentConnections;
	std::list< std::list< ConnectionType > > feedForwardDivergentConnections;
	std::list< TripletMotif * > recurrentIncompleteMotifs;
	std::list< TripletMotif * > feedForwardIncompleteMotifs;
	std::list< TripletMotif * > recurrentDivergentMotifs;
	std::list< TripletMotif * > recurrentConvergentMotifs;
	std::list< TripletMotif * > feedForwardConvergentMotifs;
	std::list< TripletMotif * > feedForwardDivergentMotifs;

	std::list< ConnectionType > recurrentIncompleteConnections1;
	recurrentIncompleteConnections1.push_back(ConnectionType(0,1));
	recurrentIncompleteConnections1.push_back(ConnectionType(1,0));
	recurrentIncompleteConnections1.push_back(ConnectionType(0,2));
	recurrentIncompleteConnections1.push_back(ConnectionType(2,0));
	std::list< ConnectionType > recurrentIncompleteConnections2;
	recurrentIncompleteConnections2.push_back(ConnectionType(1,2));
	recurrentIncompleteConnections2.push_back(ConnectionType(2,1));
	recurrentIncompleteConnections2.push_back(ConnectionType(0,2));
	recurrentIncompleteConnections2.push_back(ConnectionType(2,0));
	std::list< ConnectionType > recurrentIncompleteConnections3;
	recurrentIncompleteConnections3.push_back(ConnectionType(0,1));
	recurrentIncompleteConnections3.push_back(ConnectionType(1,0));
	recurrentIncompleteConnections3.push_back(ConnectionType(1,2));
	recurrentIncompleteConnections3.push_back(ConnectionType(2,1));
	recurrentIncompleteConnections.push_back(recurrentIncompleteConnections1);
	recurrentIncompleteConnections.push_back(recurrentIncompleteConnections2);
	recurrentIncompleteConnections.push_back(recurrentIncompleteConnections3);
	std::list< ConnectionType > feedForwardIncompleteConnections1;
	feedForwardIncompleteConnections1.push_back(ConnectionType(0,1));
	feedForwardIncompleteConnections1.push_back(ConnectionType(1,2));
	std::list< ConnectionType > feedForwardIncompleteConnections2;
	feedForwardIncompleteConnections2.push_back(ConnectionType(2,1));
	feedForwardIncompleteConnections2.push_back(ConnectionType(1,0));
	std::list< ConnectionType > feedForwardIncompleteConnections3;
	feedForwardIncompleteConnections3.push_back(ConnectionType(1,2));
	feedForwardIncompleteConnections3.push_back(ConnectionType(2,0));
	std::list< ConnectionType > feedForwardIncompleteConnections4;
	feedForwardIncompleteConnections4.push_back(ConnectionType(0,2));
	feedForwardIncompleteConnections4.push_back(ConnectionType(2,1));
	std::list< ConnectionType > feedForwardIncompleteConnections5;
	feedForwardIncompleteConnections5.push_back(ConnectionType(2,0));
	feedForwardIncompleteConnections5.push_back(ConnectionType(0,1));
	std::list< ConnectionType > feedForwardIncompleteConnections6;
	feedForwardIncompleteConnections6.push_back(ConnectionType(1,0));
	feedForwardIncompleteConnections6.push_back(ConnectionType(0,2));
	feedForwardIncompleteConnections.push_back(feedForwardIncompleteConnections1);
	feedForwardIncompleteConnections.push_back(feedForwardIncompleteConnections2);
	feedForwardIncompleteConnections.push_back(feedForwardIncompleteConnections3);
	feedForwardIncompleteConnections.push_back(feedForwardIncompleteConnections4);
	feedForwardIncompleteConnections.push_back(feedForwardIncompleteConnections5);
	feedForwardIncompleteConnections.push_back(feedForwardIncompleteConnections6);
	std::list< ConnectionType > recurrentDivergentConnections1;
	recurrentDivergentConnections1.push_back(ConnectionType(0,1));
	recurrentDivergentConnections1.push_back(ConnectionType(1,0));
	recurrentDivergentConnections1.push_back(ConnectionType(0,2));
	std::list< ConnectionType > recurrentDivergentConnections2;
	recurrentDivergentConnections2.push_back(ConnectionType(0,1));
	recurrentDivergentConnections2.push_back(ConnectionType(1,0));
	recurrentDivergentConnections2.push_back(ConnectionType(1,2));
	std::list< ConnectionType > recurrentDivergentConnections3;
	recurrentDivergentConnections3.push_back(ConnectionType(1,2));
	recurrentDivergentConnections3.push_back(ConnectionType(2,1));
	recurrentDivergentConnections3.push_back(ConnectionType(1,0));
	std::list< ConnectionType > recurrentDivergentConnections4;
	recurrentDivergentConnections4.push_back(ConnectionType(1,2));
	recurrentDivergentConnections4.push_back(ConnectionType(2,1));
	recurrentDivergentConnections4.push_back(ConnectionType(2,0));
	std::list< ConnectionType > recurrentDivergentConnections5;
	recurrentDivergentConnections5.push_back(ConnectionType(0,2));
	recurrentDivergentConnections5.push_back(ConnectionType(2,0));
	recurrentDivergentConnections5.push_back(ConnectionType(0,1));
	std::list< ConnectionType > recurrentDivergentConnections6;
	recurrentDivergentConnections6.push_back(ConnectionType(0,2));
	recurrentDivergentConnections6.push_back(ConnectionType(2,0));
	recurrentDivergentConnections6.push_back(ConnectionType(2,1));
	recurrentDivergentConnections.push_back(recurrentDivergentConnections1);
	recurrentDivergentConnections.push_back(recurrentDivergentConnections2);
	recurrentDivergentConnections.push_back(recurrentDivergentConnections3);
	recurrentDivergentConnections.push_back(recurrentDivergentConnections4);
	recurrentDivergentConnections.push_back(recurrentDivergentConnections5);
	recurrentDivergentConnections.push_back(recurrentDivergentConnections6);
	std::list< ConnectionType > recurrentConvergentConnections1;
	recurrentConvergentConnections1.push_back(ConnectionType(0,1));
	recurrentConvergentConnections1.push_back(ConnectionType(1,0));
	recurrentConvergentConnections1.push_back(ConnectionType(2,0));
	std::list< ConnectionType > recurrentConvergentConnections2;
	recurrentConvergentConnections2.push_back(ConnectionType(0,1));
	recurrentConvergentConnections2.push_back(ConnectionType(1,0));
	recurrentConvergentConnections2.push_back(ConnectionType(2,1));
	std::list< ConnectionType > recurrentConvergentConnections3;
	recurrentConvergentConnections3.push_back(ConnectionType(1,2));
	recurrentConvergentConnections3.push_back(ConnectionType(2,1));
	recurrentConvergentConnections3.push_back(ConnectionType(0,1));
	std::list< ConnectionType > recurrentConvergentConnections4;
	recurrentConvergentConnections4.push_back(ConnectionType(1,2));
	recurrentConvergentConnections4.push_back(ConnectionType(2,1));
	recurrentConvergentConnections4.push_back(ConnectionType(0,2));
	std::list< ConnectionType > recurrentConvergentConnections5;
	recurrentConvergentConnections5.push_back(ConnectionType(0,2));
	recurrentConvergentConnections5.push_back(ConnectionType(2,0));
	recurrentConvergentConnections5.push_back(ConnectionType(1,0));
	std::list< ConnectionType > recurrentConvergentConnections6;
	recurrentConvergentConnections6.push_back(ConnectionType(0,2));
	recurrentConvergentConnections6.push_back(ConnectionType(2,0));
	recurrentConvergentConnections6.push_back(ConnectionType(1,2));
	recurrentConvergentConnections.push_back(recurrentConvergentConnections1);
	recurrentConvergentConnections.push_back(recurrentConvergentConnections2);
	recurrentConvergentConnections.push_back(recurrentConvergentConnections3);
	recurrentConvergentConnections.push_back(recurrentConvergentConnections4);
	recurrentConvergentConnections.push_back(recurrentConvergentConnections5);
	recurrentConvergentConnections.push_back(recurrentConvergentConnections6);
	std::list< ConnectionType > feedForwardConvergentConnections1;
	feedForwardConvergentConnections1.push_back(ConnectionType(1,0));
	feedForwardConvergentConnections1.push_back(ConnectionType(2,0));
	std::list< ConnectionType > feedForwardConvergentConnections2;
	feedForwardConvergentConnections2.push_back(ConnectionType(0,1));
	feedForwardConvergentConnections2.push_back(ConnectionType(2,1));
	std::list< ConnectionType > feedForwardConvergentConnections3;
	feedForwardConvergentConnections3.push_back(ConnectionType(0,2));
	feedForwardConvergentConnections3.push_back(ConnectionType(1,2));
	feedForwardConvergentConnections.push_back(feedForwardConvergentConnections1);
	feedForwardConvergentConnections.push_back(feedForwardConvergentConnections2);
	feedForwardConvergentConnections.push_back(feedForwardConvergentConnections3);
	std::list< ConnectionType > feedForwardDivergentConnections1;
	feedForwardDivergentConnections1.push_back(ConnectionType(0,1));
	feedForwardDivergentConnections1.push_back(ConnectionType(0,2));
	std::list< ConnectionType > feedForwardDivergentConnections2;
	feedForwardDivergentConnections2.push_back(ConnectionType(1,0));
	feedForwardDivergentConnections2.push_back(ConnectionType(1,2));
	std::list< ConnectionType > feedForwardDivergentConnections3;
	feedForwardDivergentConnections3.push_back(ConnectionType(2,0));
	feedForwardDivergentConnections3.push_back(ConnectionType(2,1));
	feedForwardDivergentConnections.push_back(feedForwardDivergentConnections1);
	feedForwardDivergentConnections.push_back(feedForwardDivergentConnections2);
	feedForwardDivergentConnections.push_back(feedForwardDivergentConnections3);

	for(connectionIt = recurrentIncompleteConnections.begin(); connectionIt != recurrentIncompleteConnections.end(); ++connectionIt)
	{
		TripletMotif * newMotif = new TripletMotif(*connectionIt);
		recurrentIncompleteMotifs.push_back(newMotif);
	}
	for(connectionIt = feedForwardIncompleteConnections.begin(); connectionIt != feedForwardIncompleteConnections.end(); ++connectionIt)
	{
		TripletMotif * newMotif = new TripletMotif(*connectionIt);
		feedForwardIncompleteMotifs.push_back(newMotif);
	}
	for(connectionIt = recurrentDivergentConnections.begin(); connectionIt != recurrentDivergentConnections.end(); ++connectionIt)
	{
		TripletMotif * newMotif = new TripletMotif(*connectionIt);
		recurrentDivergentMotifs.push_back(newMotif);
	}
	for(connectionIt = recurrentConvergentConnections.begin(); connectionIt != recurrentConvergentConnections.end(); ++connectionIt)
	{
		TripletMotif * newMotif = new TripletMotif(*connectionIt);
		recurrentConvergentMotifs.push_back(newMotif);
	}
	for(connectionIt = feedForwardConvergentConnections.begin(); connectionIt != feedForwardConvergentConnections.end(); ++connectionIt)
	{
		TripletMotif * newMotif = new TripletMotif(*connectionIt);
		feedForwardConvergentMotifs.push_back(newMotif);
	}
	for(connectionIt = feedForwardDivergentConnections.begin(); connectionIt != feedForwardDivergentConnections.end(); ++connectionIt)
	{
		TripletMotif * newMotif = new TripletMotif(*connectionIt);
		feedForwardDivergentMotifs.push_back(newMotif);
	}

	// motifs for network analysis: one edge
	std::list< std::list< ConnectionType > > recurrentSparseConnections;
	std::list< std::list< ConnectionType > > feedForwardSparseConnections;
	std::list< TripletMotif * > recurrentSparseMotifs;
	std::list< TripletMotif * > feedForwardSparseMotifs;

	std::list< ConnectionType > recurrentSparseConnections1;
	recurrentSparseConnections1.push_back(ConnectionType(0,1));
	recurrentSparseConnections1.push_back(ConnectionType(1,0));
	std::list< ConnectionType > recurrentSparseConnections2;
	recurrentSparseConnections2.push_back(ConnectionType(1,2));
	recurrentSparseConnections2.push_back(ConnectionType(2,1));
	std::list< ConnectionType > recurrentSparseConnections3;
	recurrentSparseConnections3.push_back(ConnectionType(0,2));
	recurrentSparseConnections3.push_back(ConnectionType(2,0));
	recurrentSparseConnections.push_back(recurrentSparseConnections1);
	recurrentSparseConnections.push_back(recurrentSparseConnections2);
	recurrentSparseConnections.push_back(recurrentSparseConnections3);
	std::list< ConnectionType > feedForwardSparseConnections1;
	feedForwardSparseConnections1.push_back(ConnectionType(0,1));
	std::list< ConnectionType > feedForwardSparseConnections2;
	feedForwardSparseConnections2.push_back(ConnectionType(1,0));
	std::list< ConnectionType > feedForwardSparseConnections3;
	feedForwardSparseConnections3.push_back(ConnectionType(1,2));
	std::list< ConnectionType > feedForwardSparseConnections4;
	feedForwardSparseConnections4.push_back(ConnectionType(2,1));
	std::list< ConnectionType > feedForwardSparseConnections5;
	feedForwardSparseConnections5.push_back(ConnectionType(0,2));
	std::list< ConnectionType > feedForwardSparseConnections6;
	feedForwardSparseConnections6.push_back(ConnectionType(2,0));
	feedForwardSparseConnections.push_back(feedForwardSparseConnections1);
	feedForwardSparseConnections.push_back(feedForwardSparseConnections2);
	feedForwardSparseConnections.push_back(feedForwardSparseConnections3);
	feedForwardSparseConnections.push_back(feedForwardSparseConnections4);
	feedForwardSparseConnections.push_back(feedForwardSparseConnections5);
	feedForwardSparseConnections.push_back(feedForwardSparseConnections6);

	for(connectionIt = recurrentSparseConnections.begin(); connectionIt != recurrentSparseConnections.end(); ++connectionIt)
	{
		TripletMotif * newMotif = new TripletMotif(*connectionIt);
		recurrentSparseMotifs.push_back(newMotif);
	}
	for(connectionIt = feedForwardSparseConnections.begin(); connectionIt != feedForwardSparseConnections.end(); ++connectionIt)
	{
		TripletMotif * newMotif = new TripletMotif(*connectionIt);
		feedForwardSparseMotifs.push_back(newMotif);
	}

	// motifs for network analysis: empty motif
	std::list< ConnectionType > emtpyConnections;
	std::list< TripletMotif * > emptyMotif;
	TripletMotif * emtpyMotifPtr = new TripletMotif(emtpyConnections);
	emptyMotif.push_back(emtpyMotifPtr);

	std::map< unsigned int, std::list< TripletMotif* > > nonRedundantTriplets;
	nonRedundantTriplets[0] = recurrentLoopMotifs;
	nonRedundantTriplets[1] = directedLoopMotifs;
	nonRedundantTriplets[2] = recurrentIncompleteLoopMotifs;
	nonRedundantTriplets[3] = directedRecurrentLoopMotifs;
	nonRedundantTriplets[4] = recurrentFeedForwardConvergentMotifs;
	nonRedundantTriplets[5] = recurrentFeedForwardDivergentMotifs;
	nonRedundantTriplets[6] = feedForwardMotifs;
	nonRedundantTriplets[7] = recurrentIncompleteMotifs;
	nonRedundantTriplets[8] = feedForwardIncompleteMotifs;
	nonRedundantTriplets[9] = recurrentDivergentMotifs;
	nonRedundantTriplets[10] = recurrentConvergentMotifs;
	nonRedundantTriplets[11] = feedForwardConvergentMotifs;
	nonRedundantTriplets[12] = feedForwardDivergentMotifs;
	nonRedundantTriplets[13] = recurrentSparseMotifs;
	nonRedundantTriplets[14] = feedForwardSparseMotifs;
	nonRedundantTriplets[15] = emptyMotif;

	return nonRedundantTriplets;
}
