#include "TripletStatistic.h"
#include "Util.h"
#include "Typedefs.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseVectorSet.h"
#include <QDebug>

TripletStatistic::TripletStatistic(const NetworkProps& networkProps) :
    NetworkStatistic(networkProps){
}

/**
    Performs the actual computation based on the specified neurons.
    @param selection The selected neurons.
*/
void TripletStatistic::doCalculate(const NeuronSelection& selection){

    // Iterate over regions with postsynaptic neurons that meet the filter criteria
    const IdsPerCellTypeRegion idsPerCellTypeRegion = Util::sortByCellTypeRegionIDs(selection.Postsynaptic(), mNetwork);
    for (IdsPerCellTypeRegion::ConstIterator it = idsPerCellTypeRegion.constBegin(); it != idsPerCellTypeRegion.constEnd(); ++it) {

        // Load file with precomputed innervation data corresponding to the current region
        const CellTypeRegion cellTypeRegion = it.key();
        const QString cellTypeName = mNetwork.cellTypes.getName(cellTypeRegion.first);
        const QString regionName = mNetwork.regions.getName(cellTypeRegion.second);
        const QDir innervationDir = CIS3D::getInnervationDataDir(mNetwork.dataRoot);
        const QString innervationFile = CIS3D::getInnervationPostFileName(innervationDir, regionName, cellTypeName);
        const IdList& ids = it.value();
        SparseVectorSet* vectorSet = SparseVectorSet::load(innervationFile);
        qDebug() << "Loading" << innervationFile;

        // Iterate over postsynaptic neurons in the current region
        for (int post=0; post<ids.size(); ++post) {
            const int postId = ids[post];

            // Iterate over presynaptic neurons (exploiting axon redundancy)
            for (int pre=0; pre<selection.Presynaptic().size(); ++pre) {
                const int preId = selection.Presynaptic()[pre];
                const int mappedPreId = mNetwork.axonRedundancyMap.getNeuronIdToUse(preId);

                // Retrieve the innervation value
                const float innervationValue = vectorSet->getValue(postId, mappedPreId);

                // Perform your analysis with the innervation value.
                // Here: Record as sample to determine overall mean, variance, etc. of innervation values
                mStandardStatistic.addSample(innervationValue);
            }
        }

        // Uncomment, when integrating the statistic into the webframework
        // reportUpdate();
    }

    // Uncomment, when integrating the statistic into the webframework
    // reportComplete();
}

//#################################################################################################################
//#################################################################################################################
//#################################################################################################################

/*
void analyzeTripletMotifs(unsigned int column1, unsigned int type1, unsigned int column2, unsigned int type2, unsigned int column3, unsigned int type3, unsigned int nrOfTriplets, int nrOfLoops, const char * outputfilename)
{
	unsigned int pretype1, posttype1, pretype2, posttype2, pretype3, posttype3;
	helper::getTypeIDs(type1, pretype1, posttype1);
	helper::getTypeIDs(type2, pretype2, posttype2);
	helper::getTypeIDs(type3, pretype3, posttype3);

	// Take care of VPM
	std::vector<int> VPMCellID;
	if (posttype1==0)
	{
		posttype1 = pretype1;
		VPMCellID.push_back(0);
	}
	if (posttype2==0)
	{
		posttype2 = pretype2;
		VPMCellID.push_back(1);
	}
	if (posttype3==0)
	{
		posttype3 = pretype3;
		VPMCellID.push_back(2);
	}

	std::string pre1Str = std::string(int2CelltypeLabels[pretype1]) + "_" + std::string(int2ColumnLabels[column1]);
	std::string post1Str = std::string(int2CelltypeLabels[posttype1]) + "_" + std::string(int2ColumnLabels[column1]);
	std::string pre2Str = std::string(int2CelltypeLabels[pretype2]) + "_" + std::string(int2ColumnLabels[column2]);
	std::string post2Str = std::string(int2CelltypeLabels[posttype2]) + "_" + std::string(int2ColumnLabels[column2]);
	std::string pre3Str = std::string(int2CelltypeLabels[pretype3]) + "_" + std::string(int2ColumnLabels[column3]);
	std::string post3Str = std::string(int2CelltypeLabels[posttype3]) + "_" + std::string(int2ColumnLabels[column3]);

	// Add CellTypeColumn Combination to listNames (but add each combination only once)
	std::vector<std::pair < std::string,std::string > > listNames;
	listNames.push_back(std::pair<std::string,std::string>(pre1Str,post1Str));

	std::vector<std::pair < std::string,std::string > >::iterator it = find(listNames.begin(), listNames.end(), std::pair<std::string,std::string>(pre2Str,post2Str));
	if (it == listNames.end())
		listNames.push_back(std::pair<std::string,std::string>(pre2Str,post2Str));

	it = find(listNames.begin(), listNames.end(), std::pair<std::string,std::string>(pre3Str,post3Str));
	if (it == listNames.end())
		listNames.push_back(std::pair<std::string,std::string>(pre3Str,post3Str));

	std::cout << "-----------------------------------" << std::endl;
	std::cout << "Start computing triplet motifs for " << std::endl;
	std::cout << "	Cell1: " << pre1Str << " " << post1Str << std::endl;
	std::cout << "	Cell2: " << pre2Str << " " << post2Str << std::endl;
	std::cout << "	Cell3: " << pre3Str << " " << post3Str << std::endl;

	// Read in Header of Original InnervationMatrix
	connectionMatrixReader * connectionReader = new connectionMatrixReader();
	ConnectionMatrix * connectome = new ConnectionMatrix;
	connectionReader->readConnectionMatrixHeader(connectome);
	connectome->simpleMatrix = true;

	// Add Innervation Values
	for (int i = 0; i<listNames.size(); ++i) // Presynapse
	{
		std::pair <std::string,std::string> tmp1 = listNames.at(i);

		for (int j = 0; j<listNames.size(); ++j) // Postsynapse
		{
			// Skip if postsynapse is VPM
			std::vector<int>::iterator itVPMID = find(VPMCellID.begin(), VPMCellID.end(), j);
			if (itVPMID != VPMCellID.end())
				continue;

			std::pair <std::string,std::string> tmp2 = listNames.at(j);

			std::string filename = "/nas1/Data_daniel/Network/Axon2/InnervationMatrix/SimpleInnervationMatrix/" + tmp1.first +
					"_" + tmp2.second + ".csv";
			connectionReader->addConnectionMatrixFromCSV(filename.c_str(),connectome);
			std::cout << " added Innervation values of " << filename << std::endl;
		}
	}

	for (int i = 0; i < nrOfLoops; ++i)
	{
		MatrixAnalyzer * matrixAnalysis = new MatrixAnalyzer(connectome);
		std::pair< std::vector< Profile * >, std::vector< Profile * > > tripletMotifVecPair = matrixAnalysis->computeTripletMotif3Cells(column1, pretype1, posttype1, column2, pretype2, posttype2, column3, pretype3, posttype3, nrOfTriplets);

		std::stringstream ss;
		ss << i;
		std::string motifOutName = std::string(outputfilename) + "_loop" + ss.str();

		writeMotifHistograms(motifOutName.c_str(),tripletMotifVecPair);
		delete matrixAnalysis, tripletMotifVecPair;
	}

	std::cout << "Finished computing triplet motifs" << std::endl;
	std::cout << "-----------------------------------" << std::endl;
	delete connectome;
	delete connectionReader;
}
*/

//#################################################################################################################
//#################################################################################################################
//#################################################################################################################

/*
std::pair< std::vector< Profile * >, std::vector< Profile * > > MatrixAnalyzer::computeTripletMotif3Cells(unsigned int column1, unsigned int pretype1, unsigned int posttype1, unsigned int column2, unsigned int pretype2, unsigned int posttype2, unsigned int column3, unsigned int pretype3, unsigned int posttype3, unsigned int nrOfTriplets)
{
	std::cout << "Analyzing triplet motif distribution for " << nrOfTriplets << " triplets..." << std::endl;
	std::pair< std::vector< Profile * >, std::vector< Profile * > > tripletMotifVecPair;

	SelectionType preSelection1 = connectome->getPreColumnCelltypeSelection(column1, pretype1);
	SelectionType postSelection1 = connectome->getPostColumnCelltypeSelection(column1, posttype1);

	SelectionType preSelection2 = connectome->getPreColumnCelltypeSelection(column2, pretype2);
	SelectionType postSelection2 = connectome->getPostColumnCelltypeSelection(column2, posttype2);

	SelectionType preSelection3 = connectome->getPreColumnCelltypeSelection(column3, pretype3);
	SelectionType postSelection3 = connectome->getPostColumnCelltypeSelection(column3, posttype3);

	// If postsynaptic cell type does not exist (in case of VPM), take presynapse!
	if (postSelection1.size()==0 && preSelection1.size()>0)
	{
		postSelection1 = connectome->getPreColumnCelltypeSelection(column1, pretype1);
	}
	if (postSelection2.size()==0 && preSelection2.size()>0)
	{
		postSelection2 = connectome->getPreColumnCelltypeSelection(column2, pretype2);
	}
	if (postSelection3.size()==0 && preSelection3.size()>0)
	{
		postSelection3 = connectome->getPreColumnCelltypeSelection(column3, pretype3);
	}

	// Check for potential errors/mismatches
	if (preSelection1.size() != postSelection1.size() || preSelection1.size()==0 || postSelection1.size()==0)
	{
		std::cout << "ERROR: Pre- and Postsynaptic Selection1 are not equal sized " << preSelection1.size() << " " << postSelection1.size() << std::endl;
		return tripletMotifVecPair;
	}
	if (preSelection2.size() != postSelection2.size() || preSelection2.size()==0 || postSelection2.size()==0)
	{
		std::cout << "ERROR: Pre- and Postsynaptic Selection2 are not equal sized " << preSelection2.size() << " " << postSelection2.size() << std::endl;
		return tripletMotifVecPair;
	}
	if (preSelection3.size() != postSelection3.size() || preSelection3.size()==0 || postSelection3.size()==0)
	{
		std::cout << "ERROR: Pre- and Postsynaptic Selection3 are not equal sized " << preSelection3.size() << " " << postSelection3.size() << std::endl;
		return tripletMotifVecPair;
	}

	std::map< unsigned int, std::list< TripletMotif * > > tripletMotifs = initializeNonRedundantTripletMotifs();

	std::list< CellTriplet * > triplets = initializeNonRedundantCellTriplets(preSelection1, postSelection1, preSelection2, postSelection2, preSelection3, postSelection3, nrOfTriplets);
	std::cout << "Computing probability of occurrence for " << triplets.size() << " triplets..." << std::endl;
	tripletMotifVecPair = processTripletsAll64(triplets, tripletMotifs);

	return tripletMotifVecPair;
}
*/

//#################################################################################################################
//#################################################################################################################
//#################################################################################################################

/*
std::list< CellTriplet * > MatrixAnalyzer::initializeNonRedundantCellTriplets(SelectionType preSelection1, SelectionType postSelection1, SelectionType preSelection2, SelectionType postSelection2, SelectionType preSelection3, SelectionType postSelection3, unsigned int nrOfTriplets)
{
	std::list< CellTriplet * > triplets;
	// Check Input
	if (preSelection1.size() != postSelection1.size() || preSelection1.size()==0 || postSelection1.size()==0)
	{
		std::cout << "ERROR: Selection1 are not equal sized or empty " << preSelection1.size() << " " << postSelection1.size() << std::endl;
		return triplets;
	}
	if (preSelection2.size() != postSelection2.size() || preSelection2.size()==0 || postSelection2.size()==0)
	{
		std::cout << "ERROR: Selection2 are not equal sized or empty " << preSelection2.size() << " " << postSelection2.size() << std::endl;
		return triplets;
	}
	if (preSelection3.size() != postSelection3.size() || preSelection3.size()==0 || postSelection3.size()==0)
	{
		std::cout << "ERROR: Selection3 are not equal sized or empty " << preSelection3.size() << " " << postSelection3.size() << std::endl;
		return triplets;
	}

	std::cout << "Selecting " << nrOfTriplets << " cell triplets from connection matrix..." << std::endl;

	std::srand(std::time(NULL));
	std::time_t start = std::time(0);

	const unsigned int NMAX1 = std::max(preSelection1.size(), postSelection1.size());
	const unsigned int NMAX2 = std::max(preSelection2.size(), postSelection2.size());
	const unsigned int NMAX3 = std::max(preSelection3.size(), postSelection3.size());

	// Compute Average Innervation Values
	double avgConvergenceValues[3][3];
	avgConvergenceValues[0][0] = connectome->getAverageConvergenceOptimized(preSelection1,postSelection1);
	avgConvergenceValues[0][1] = connectome->getAverageConvergenceOptimized(preSelection1,postSelection2);
	avgConvergenceValues[0][2] = connectome->getAverageConvergenceOptimized(preSelection1,postSelection3);
	avgConvergenceValues[1][0] = connectome->getAverageConvergenceOptimized(preSelection2,postSelection1);
	avgConvergenceValues[1][1] = connectome->getAverageConvergenceOptimized(preSelection2,postSelection2);
	avgConvergenceValues[1][2] = connectome->getAverageConvergenceOptimized(preSelection2,postSelection3);
	avgConvergenceValues[2][0] = connectome->getAverageConvergenceOptimized(preSelection3,postSelection1);
	avgConvergenceValues[2][1] = connectome->getAverageConvergenceOptimized(preSelection3,postSelection2);
	avgConvergenceValues[2][2] = connectome->getAverageConvergenceOptimized(preSelection3,postSelection3);

	std::list< std::vector< unsigned int > > usedTriplets;
	while(triplets.size() < nrOfTriplets)
	{
		unsigned int index1, index2, index3;
		unsigned int preCell1, preCell2, preCell3, postCell1, postCell2, postCell3;

		// Cell1 randomly drawn from Seleciton1
		index1 = std::rand()%NMAX1;
		preCell1 = preSelection1[index1];
		postCell1 = postSelection1[index1];

		// Cell2 randomly drawn from Selection2
		index2 = std::rand()%NMAX2;
		preCell2 = preSelection2[index2];
		postCell2 = postSelection2[index2];

		// Cell3 randomly drawn from Selection3
		index3 = std::rand()%NMAX3;
		preCell3 = preSelection3[index3];
		postCell3 = postSelection3[index3];

		// If drawn CellIDs are identical, draw again
		if(preCell1 == preCell2 || preCell1 == preCell3 || preCell2 == preCell3)
		{
			continue;
		}

		CellTriplet * newTriplet = new CellTriplet(preCell1, preCell2, preCell3, postCell1, postCell2, postCell3);
		newTriplet->setInnervationMatrix(connectome);

		// set average convergence
		std::vector< std::vector< double > > avgConvergence;
		for(int ii = 0; ii < 3; ++ii)
		{
			std::vector< double > emptyRow;
			avgConvergence.push_back(emptyRow);
			for(int jj = 0; jj < 3; ++jj)
			{
				if(ii == jj)
				{
					avgConvergence[ii].push_back(0);
					continue;
				}

				double convergence = avgConvergenceValues[ii][jj];
				avgConvergence[ii].push_back(convergence);
			}
		}
		newTriplet->setAverageConvergenceMatrix(avgConvergence);
		triplets.push_back(newTriplet);
	}
	std::cout << std::endl;
	return triplets;
}
*/

//#################################################################################################################
//#################################################################################################################
//#################################################################################################################

/*
double ConnectionMatrix::getAverageConvergenceOptimized(SelectionType preSelection, SelectionType postSelection)
{
	unsigned int loopSelection = preSelection.size()*postSelection.size();
	unsigned int loopMatrix = matrix.size() + preSelection.size();

	if (loopMatrix<loopSelection)
	{
		return getAverageConvergenceIterateMatrix(preSelection, postSelection);
	}
	else
	{
		if (loopSelection>(6000^2)) // parallization only makes sense for high number of iterations (otherwise initializing takes too long)
		{
			return getAverageConvergenceIterateSelection(preSelection, postSelection);
		}
		else
		{
			return getAverageConvergence(preSelection, postSelection);
		}
	}

}
*/

//#################################################################################################################
//#################################################################################################################
//#################################################################################################################

/*
double ConnectionMatrix::getAverageConvergenceIterateMatrix(SelectionType preSelection, SelectionType postSelection)
{
	double convergence = 0;
	std::map<unsigned int, unsigned int> PreCellIDCounter;

	if (simpleMatrix)
	{
		for(int i = 0; i < preSelection.size(); ++i)
		{
			std::map<unsigned int, unsigned int>::iterator it = preDuplicatedIDtoSingleID.find(preSelection[i]);
			if (it == preDuplicatedIDtoSingleID.end())
			{
				std::cout << "ERROR! PreCellID " << preSelection[i] << " was not found!" << std::endl;
				std::cout << "	ColumnID: " << IDColumnCelltypeMap[preSelection[i]].first;
				std::cout << "	CellTypeID: " << IDColumnCelltypeMap[preSelection[i]].second << std::endl;
			}
			else
			{
				if (PreCellIDCounter.count(it->second)>0)
				{
					PreCellIDCounter[it->second]++;
				}
				else
				{
					PreCellIDCounter.insert(std::pair< unsigned int, unsigned int >(it->second,1));
				}
			}
		}
	}

	for (std::map< MatrixIndexType, float >::iterator it = matrix.begin(); it != matrix.end(); ++it)
	{
		unsigned int preCellID = it->first.first;
		unsigned int postCellID = it->first.second;

		if (std::find(postSelection.begin(), postSelection.end(), postCellID) != postSelection.end())
		{
			if (simpleMatrix)
			{
				if (PreCellIDCounter.count(preCellID)>0)
				{
					float innervation = it->second;
					convergence = convergence + (1 - exp(-1*innervation))*PreCellIDCounter[preCellID];
				}
			}
			else if (std::find(preSelection.begin(), preSelection.end(), preCellID) != preSelection.end())
			{
				float innervation = it->second;
				convergence = convergence + (1 - exp(-1*innervation));
			}
		}
	}

	convergence = convergence / (preSelection.size()*postSelection.size());
	return convergence;
}
*/

//#################################################################################################################
//#################################################################################################################
//#################################################################################################################

/*
std::pair< std::vector< Profile * >, std::vector< Profile * > > MatrixAnalyzer::processTripletsAll64(std::list< CellTriplet * > triplets, std::map< unsigned int, std::list< TripletMotif * > > tripletMotifs)
{
	Profile * motifHistogram = new Profile(1);
	Profile * motifHistogramAvg = new Profile(1);

	std::vector< Profile * > tripletMotifVec;
	std::vector< Profile * > tripletMotifVecAvg;

	std::list< CellTriplet * >::const_iterator tripletsIt;

	for(tripletsIt = triplets.begin(); tripletsIt != triplets.end(); ++tripletsIt)
	{
		// pick current cell triplet
		CellTriplet * currentTriplet = *tripletsIt;

		// Go through all possible triplet motifs
		int c = 0;
		for(int ii = 0; ii < tripletMotifs.size(); ++ii) // 16 Main Motifs
		{
			std::list< TripletMotif * > motifList = tripletMotifs[ii];
			std::list< TripletMotif * >::const_iterator motifListIt;

			// Go through all possible motifs of respective triplet motif and sum up probabilities
			for(motifListIt = motifList.begin(); motifListIt != motifList.end(); ++motifListIt)
			{
				TripletMotif * currentMotif = *motifListIt;
				double motifProb = currentMotif->computeOccurrenceProbability(currentTriplet->innervation);
				double motifProbAvg = currentMotif->computeOccurrenceProbabilityGivenInputProbability(currentTriplet->avgConvergence);

				motifHistogram->addSegment(motifProb, c);
				motifHistogramAvg->addSegment(motifProbAvg, c);
				c++;
			}
		}
		Profile * intermediateHist = new Profile(1);
		intermediateHist->addProfile(motifHistogram);
		tripletMotifVec.push_back(intermediateHist);

		Profile * intermediateHistAvg = new Profile(1);
		intermediateHistAvg->addProfile(motifHistogramAvg);
		tripletMotifVecAvg.push_back(intermediateHistAvg);
	}

	int numMotifs = motifHistogram->getProfile()->size();

	// normalize
	for(int ii = 0; ii < tripletMotifVec.size(); ++ii)
	{
		for(int jj = 0; jj < numMotifs; ++jj)
		{
			Profile * tmpHist = tripletMotifVec[ii];
			double histValue = tmpHist->getProfile()->at(jj);
			histValue /= (double)(ii+1);
			tmpHist->getProfile()->at(jj) = histValue;
		}
	}

	// normalize Average Profiles
	for(int ii = 0; ii < tripletMotifVecAvg.size(); ++ii)
	{
		for(int jj = 0; jj < numMotifs; ++jj)
		{
			Profile * tmpHist = tripletMotifVecAvg[ii];
			double histValue = tmpHist->getProfile()->at(jj);
			histValue /= (double)(ii+1);
			tmpHist->getProfile()->at(jj) = histValue;
		}
	}

	// Clean up
	for(int ii = 0; ii < tripletMotifs.size(); ++ii)
	{
		std::list< TripletMotif * > motifList = tripletMotifs[ii];
		std::list< TripletMotif * >::iterator motifListIt;
		for(motifListIt = motifList.begin(); motifListIt != motifList.end(); ++motifListIt)
		{
			delete *motifListIt;
		}
	}

	std::list< CellTriplet * >::iterator tripletsCleanIt;
	for(tripletsCleanIt = triplets.begin(); tripletsCleanIt != triplets.end(); ++tripletsCleanIt)
	{
		delete *tripletsCleanIt;
	}
	return std::make_pair(tripletMotifVec,tripletMotifVecAvg);
}
*/

/**
    Adds the result values to a JSON object
    @param obj JSON object to which the values are appended
*/
void TripletStatistic::doCreateJson(QJsonObject& /*obj*/) const {
    // Implement when integrating the statistic into the webframework
    // refer to shared/InnervationStatistic.cpp as reference
}

/**
    Writes the result values to file stream (CSV).
    @param out The file stream to which the values are written.
    @param sep The separator between parameter name and value.
*/
void TripletStatistic::doCreateCSV(QTextStream& /*out*/, const QChar /*sep*/) const {
    // Implement when integrating the statistic into the webframework
    // refer to shared/InnervationStatistic.cpp as reference
}

/**
    Returns internal result, for testing purposes.
    @return The statistic.
*/
Statistics TripletStatistic::getResult() const{
    return mStandardStatistic;
}
