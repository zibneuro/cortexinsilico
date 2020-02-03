#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif
#include "../../common/typedefs.h"
#include "../../common/basics.h"
#include "../../common/amiraReader.h"
#include "../../common/barrel_field.h"
#include "../../Interneuron/src/helper.h"
#include <set>
#include <utility>

struct edgeInfo
{
	int edgeID;
	double edgeLength;
	double boutons;
	double spines;
	int edgeLabel;
	bool connectedToSoma;
};

AmiraSpatialGraph * getSpatialGraphsInsideBounds(AmiraSpatialGraph * sg,
													double bounds[6]);
AmiraSpatialGraph * getSpatialGraphsInsideBounds(AmiraSpatialGraph * sg,
													double bounds[6],
													bool printMessages);

std::vector< edgeInfo > getStructureInsideBounds(AmiraSpatialGraph * sg);
// Updated Version that computes bouton density voxel-wise
std::vector< edgeInfo > getStructureInsideBounds(AmiraSpatialGraph * sg,
														int CellTypeID,
														int laminarPosition);

std::vector< int > getCellIDList(const char * filename);


bool addIntersectionPointsToSG(AmiraSpatialGraph * sg, double bounds[6]);
bool isPtInBothLists(std::list< double* > ptList1, std::list< double* >ptList2);
bool IsPointOnBorder(double pt[3], double bounds[6]);
void addEdge(std::list< double * > newEdgePointCoordinates, std::list< double > newRadList,
							int edgeLabel, AmiraSpatialGraph * sg);
void writeEdgeInfoVector(std::vector< edgeInfo > edgeInfoVector, const char * outputfilename);
void computeForDenseModel(std::vector<int> cellIDList, double origin[3], int dim[3],
							const char * outputpath);
void computeForDenseModelInsidevS1(std::vector<int> cellIDList, const char * outputpath);
void getDensityFactors(int CellTypeID, std::map<int, double>& boutonDensity,
							std::map<int, double>& spineDensity);
void computeValidVoxelsInsidevS1(const char * outputpath, double voxelSize);
void printUsage();

// BarrelField, used to compute boutons in INFRA, GRAN and SUPRA layers
BarrelField * SBF;

/* old functions that compute intersection points with laminar surfaces
 * In NeuroNet bouton density is computed vixel-wise
 * So to compare results with NeuroNet no need to compute the intersection */
std::vector< edgeInfo > getStructureInsideBounds(AmiraSpatialGraph * sg,
														int CellTypeID);
double computeBoutons(std::list< double* > pts,
						std::map<int, double> boutonDensity);
double computerIntersectionPt(int currLaminar, int prevLaminar, double intersectPt[3],
		double * currPt, double * prevPt, double signWeigth);

/* Extract Parts of a SpatialGraph that are within a defined bounding box */
int main(int argc, const char** argv)
{
	/* ./GetCellPartsInBox [Inputfilename] [Outputfilename] [xmin] [xmax] [ymin] [ymax] [zmin] [zmax]
	 * 		Writes out cell parts as spatial graph */
	if(argc == 9)
	{
		// Default bounds
		double bounds[6] = {-50.0, 50.0, -50.0, 50.0, -50.0, 50.0};

		const char * inputfilename = argv[1];
		const char * outputfilename = argv[2];

		// Read in Bounding Box [xmin xmax ymin ymax zmin zmax]
		for (int i=0; i<6; i++)
		{
			bounds[i] = atof(argv[i+3]);
		}

		for (int i=0; i<3; i++)
		{
			if (bounds[i*2+1]<=bounds[i*2])
			{
				std::cout << "ERROR! BoundingBox is not set properly!" << std::endl;
				std::cout << bounds[i*2] << " is not smaller than " << bounds[i*2+1] << std::endl;
				printUsage();
				return 0;
			}
		}

		// Read in Spatial Graph
		AmiraSpatialGraph * sg = helper::getSpatialGraph(inputfilename);

		// Extract SpatialGraph Parts that are within Bounds
		bool sgWithinBounds = addIntersectionPointsToSG(sg,bounds);
		if (sgWithinBounds)
		{
			AmiraSpatialGraph * SpatialGraphsInsideBounds = getSpatialGraphsInsideBounds(sg,bounds);

			// Only write Spatial Graph if it contains at least one edge (otherwise it's empty)
			if (SpatialGraphsInsideBounds->getNumberOfEdges()<1)
			{
				std::cout << "ERROR! Something is off. SpatialGraph is within bounds";
				std::cout << " but no edges were extracted!" << std::endl;
			}

			// Save SpatialGraph Parts
			// NOTE: Connectivity between Edges is not preserved!
			Reader * amWriterCut = new Reader(outputfilename, outputfilename);
			amWriterCut->setSpatialGraph(SpatialGraphsInsideBounds);
			amWriterCut->writeSpatialGraphFile();
			return 0;
		}
		else
		{
			std::cout << "No Edges found in volume. No SpatialGraph written to Disk!" << std::endl;
			return 0;
		}
	}
	/* ./GetCellPartsInBox [Inputfilename] [Outputfilename] [xmin] [xmax] [ymin] [ymax] [zmin] [zmax] [dummyvalue]
	 * 		writes information about spatial graph */
	else if(argc == 10)
	{
		// Default bounds
		double bounds[6] = {-50.0, 50.0, -50.0, 50.0, -50.0, 50.0};

		const char * inputfilename = argv[1];
		const char * outputfilename = argv[2];

		// Read in Bounding Box [xmin xmax ymin ymax zmin zmax]
		for (int i=0; i<6; i++)
		{
			bounds[i] = atof(argv[i+3]);
		}

		for (int i=0; i<3; i++)
		{
			if (bounds[i*2+1]<=bounds[i*2])
			{
				std::cout << "ERROR! BoundingBox is not set properly!" << std::endl;
				std::cout << bounds[i*2] << " is not smaller than " << bounds[i*2+1] << std::endl;
				printUsage();
				return 0;
			}
		}

		// Read in Spatial Graph
		AmiraSpatialGraph * sg = helper::getSpatialGraph(inputfilename);
		// Extract SpatialGraph Parts that are within Bounds
		bool sgWithinBounds = addIntersectionPointsToSG(sg,bounds);
		if (sgWithinBounds)
		{
			AmiraSpatialGraph * SpatialGraphsInsideBounds = getSpatialGraphsInsideBounds(sg,bounds);

			// Only write Spatial Graph if it contains at least one edge (otherwise it's empty)
			if (SpatialGraphsInsideBounds->getNumberOfEdges()<1)
			{
				std::cout << "ERROR! Something is off. SpatialGraph is within bounds";
				std::cout << " but no edges were extracted!" << std::endl;
			}

			std::vector< edgeInfo > edgeInfoVector = getStructureInsideBounds(SpatialGraphsInsideBounds);
			if (edgeInfoVector.size() > 0)
			{
				writeEdgeInfoVector(edgeInfoVector,outputfilename);
			}
			else
			{
				std::cout << "ERROR! Something is off. edgeInfoVector is empty" << std::endl;
			}
		}
	} /* Extract Neurite Snippets innervating voxels given a list of CELLIDs */
	else if (argc == 2) //
	{
		if (atoi(argv[1])==1 || atoi(argv[1])==0) // Dendrites (based on ID)
		{
			double origin[3] = {-200, 300, -1000};
			int dim[3] = {4, 4, 32};
			const char * filenamecellIDs = "/home/dudvary/tasks/NetworkSimulator/results/cellIDs_ExcDend.txt";
			const char * outputpath = "/home/dudvary/tasks/NetworkSimulator/results_dendriteSnippets/";

			std::vector<int> cellIDList = getCellIDList(filenamecellIDs);
			computeForDenseModel(cellIDList, origin, dim, outputpath);
		}
		if (atoi(argv[1])==2 || atoi(argv[1])==0) // Axons (based on ID)
		{
			double origin[3] = {-200, 300, -1000};
			int dim[3] = {4, 4, 32};
			const char * filenamecellIDs = "/home/dudvary/tasks/NetworkSimulator/results/cellIDs_ExcAxon.txt";
			const char * outputpath = "/home/dudvary/tasks/NetworkSimulator/results_axonSnippets/";

			std::vector<int> cellIDList = getCellIDList(filenamecellIDs);
			computeForDenseModel(cellIDList, origin, dim, outputpath);
		}
		if (atoi(argv[1])==3) // Axons (based on ID)
		{
			std::cout << "Not implemented yet!" << std::endl;
			const char * filenamecellIDs = "/home/dudvary/tasks/NetworkSimulator/results_v2/cellIDs_ExcAxon.txt";
			const char * outputpath = "/home/dudvary/tasks/NetworkSimulator/results_axonSnippets_v2/";

			std::vector<int> cellIDList = getCellIDList(filenamecellIDs);
			computeForDenseModelInsidevS1(cellIDList, outputpath);
		}
		if (atoi(argv[1])==4) // Dendrites (based on ID)
		{
			std::cout << "Not implemented yet!" << std::endl;
			const char * filenamecellIDs = "/home/dudvary/tasks/NetworkSimulator/results_v2/cellIDs_ExcDend.txt";
			const char * outputpath = "/home/dudvary/tasks/NetworkSimulator/results_dendriteSnippets_v2/";

			std::vector<int> cellIDList = getCellIDList(filenamecellIDs);
			computeForDenseModelInsidevS1(cellIDList, outputpath);
		}

		if (atoi(argv[1])>4 || atoi(argv[1])<0)
		{
			std::cout << "ERROR! Wrong input ID! Use 0 for dendrites and axons, 1 for only dendrites, 2 for only axons!" << std::endl;
			printUsage();
		}
	}
	else
	{
		std::cout << "ERROR! Wrong number of input arguments!" << std::endl;
		printUsage();
		return 0;
	}
}

void computeValidVoxelsInsidevS1(const char * outputpath, double voxelSize)
{
	double BB[6] = {-1200.0, 1500.0, -850.0, 1600.0, -1550.0, 800.0};
	int voxelID = 0;

	// Check whether edges of bounding box are outside of vS1, if so everything is good!
	for (int x = 0; x<2; x++)
	{
		for (int y = 2; y<4; y++)
		{
			for (int z = 4; z<6; z++)
			{
				double center[3] = {BB[x]+ voxelSize/2,
									BB[y]+ voxelSize/2,
									BB[z]+ voxelSize/2};

				if (SBF->isInsideS1(center))
				{
					std::cout << "ERROR! Bounding Box is not set properly!" << std::endl;
					std::cout << "Center = [" << center[0] << "," << center[1] << ",";
					std::cout << center[2] << "] is inside of vS1!" << std::endl;
					return;
				}
			}
		}
	}

	for (int x = 0; x<=((BB[1]-BB[0])/voxelSize); x++)
	{
		for (int y = 0; y<=((BB[3]-BB[2])/voxelSize); y++)
		{
			for (int z = 0; z<=((BB[5]-BB[4])/voxelSize); z++)
			{
				double center[3] = {BB[0]+ x*voxelSize+voxelSize/2,
									BB[2]+ y*voxelSize+voxelSize/2,
									BB[4]+ z*voxelSize+voxelSize/2};

				if (SBF->isInsideS1(center))
				{
					double piaDist = SBF->piaDistance(center);
					int insideColumn = SBF->insideColumn(center);
					int nearestColumn = SBF->closestBarrel(center);
					int laminarPos = SBF->laminarPosition(center);
					voxelID++;
				}
			}
		}
	}

	/* TO BE IMPLEMENTED
	 * create a struct with all the information for each voxel
	 * and save it as csv file
	 */
	std::cout << "YOU ARE HERE!" << std::endl;
}

void computeForDenseModelInsidevS1(std::vector<int> cellIDList, const char * outputpath)
{
	bool DEBUG = false;
	SBF = new BarrelField;
	double voxelSize = 50.0;
	computeValidVoxelsInsidevS1(outputpath,voxelSize);

//	const char * spatialGraphSetFilename =
//			"/nas1/Data_regger/AXON_SAGA/Axon2/NeuroNet/cache_Vincent_complete_final/Realization20151007_ascii/Morphologies.am";
//	std::vector< unsigned int > originalGraphIndices;
//	std::vector< unsigned int > cellTypeIDs;
//	std::vector< double * > spatialGraphTransforms;
//	std::vector< std::string > originalGraphFiles;
//	std::map< unsigned int, std::string > cellTypeIDLabels;
//	Reader::readSpatialGraphSetFile(spatialGraphSetFilename, originalGraphIndices,
//			cellTypeIDs, spatialGraphTransforms, originalGraphFiles, cellTypeIDLabels);
//
//	std::cout << "Successfully read in SpatialGraphSet file." << std::endl;
//	std::cout << "Processing individual spatial graphs now ... " << std::endl;
//	std::cout << "Results are written in " << outputpath << std::endl;
//	std::cout << "---------------------------------------------" << std::endl;
//
//	for (int id = 0; id<cellIDList.size(); id++)
//	{
//		int cellID = cellIDList[id];
//		int voxelID = 1;
//
//		// Set up text file writer
//		std::stringstream stream1;
//		stream1 << cellID;
//		std::string txtfilename = std::string(outputpath) + stream1.str() + "_snippets.csv";
//
//		std::ofstream TXTWriter;
//		bool openedTxtWriter = false;
//
//		// Get Filename
//		std::string originalName = originalGraphFiles[originalGraphIndices[cellID]];
//		originalName = originalName.substr(1, originalName.size()-2);
//		std::string loadName = helper::getRootFromPath(spatialGraphSetFilename) + originalName;
//
//		// Load Spatial Graph
//		Reader * spatialGraphReader = new Reader(loadName.c_str(), loadName.c_str());
//		spatialGraphReader->readSpatialGraphFile(0);
//		AmiraSpatialGraph * sgOriginal = spatialGraphReader->getSpatialGraph();
//
//		// If axon, remove Dendrites
//		if (helper::isAxonCellType(cellTypeIDs[cellID]))
//		{
//			sgOriginal->removeLabel(Dendrite);
//			sgOriginal->removeLabel(ApicalDendrite);
//			sgOriginal->removeLabel(BasalDendrite);
//		}
//		else // otherwise remove axon, and apply transformation
//		{
//			sgOriginal->removeLabel(Axon);
//
//			// Apply Transformation
//			sgOriginal->setTransformation(helper::amiraToVTKTransform(spatialGraphTransforms[cellID]));
//			sgOriginal->applyTransformation();
//		}
//
//		if (DEBUG)
//		{
//			std::cout << "Read in " << loadName << std::endl;
//			std::cout << "CellID = " << cellID << std::endl;
//			std::cout << "CellType = " << cellTypeIDs[cellID];
//			std::cout << " (" << cellTypeIDLabels[cellTypeIDs[cellID]] << ")" << std::endl;
//			std::cout << "IsAxonCellType = " << helper::isAxonCellType(cellTypeIDs[cellID]) << std::endl;
//		}

		// Go through all voxels
//		for (int z = 0; z<dim[2]; z++)
//		{
//			for (int y = 0; y<dim[1]; y++)
//			{
//				for (int x = 0; x<dim[0]; x++, voxelID++)
//				{
//					double bounds[6] = {origin[0]+ x*voxelSize, origin[0]+ (x+1)*voxelSize,
//							origin[1]+ y*voxelSize, origin[1]+ (y+1)*voxelSize,
//							origin[2]+ z*voxelSize, origin[2]+ (z+1)*voxelSize};
//
//					if (DEBUG)
//					{
//						std::cout << "-------------------------" << std::endl;
//						std::cout << "Current bounds = [" << voxelID << "; ";
//						std::cout << bounds[0] << " " << bounds[1] << ", ";
//						std::cout << bounds[2] << " " << bounds[3] << ", ";
//						std::cout << bounds[4] << " " << bounds[5] << "]" << std::endl;
//						//std::cout << "#SpatialGraphEdges = "  << sgOriginal->getNumberOfEdges() << std::endl;
//					}
//
//					// Extract SpatialGraph Parts that are within Bounds
//					bool sgWithinBounds = addIntersectionPointsToSG(sgOriginal,bounds);
//
//					if (sgWithinBounds)
//					{
//						AmiraSpatialGraph * SpatialGraphsInsideBounds = getSpatialGraphsInsideBounds(sgOriginal,bounds,false);
//
//						// Only write Spatial Graph if it contains at least one edge (otherwise it's empty)
//						// Could be that there is an edge with only one point
//						if (SpatialGraphsInsideBounds->getNumberOfEdges()>0)
//						{
//							double centerPt[3] = {bounds[0] + voxelSize/2, bounds[2] + voxelSize/2, bounds[4] + voxelSize/2};
//							int laminarPos = SBF->laminarPosition(centerPt);
//
//							std::vector< edgeInfo > edgeInfoVector = getStructureInsideBounds(SpatialGraphsInsideBounds,cellTypeIDs[cellID],laminarPos);
//							if (edgeInfoVector.size() > 0)
//							{
//								if (!openedTxtWriter)
//								{
//									TXTWriter.open(txtfilename.c_str());
//									if(TXTWriter.fail())
//									{
//										std::cout << "ERROR! computeForDenseModel: Writing " << txtfilename << " failed! " << std::endl;
//										return;
//									}
//									TXTWriter << "voxelID,edgeID,edgeLength,edgeLabel,connectedToSoma,boutons,spines," << std::endl;
//									openedTxtWriter = true;
//								}
//
//								// Print to file
//								for (std::vector<edgeInfo>::iterator it=edgeInfoVector.begin(); it!=edgeInfoVector.end(); ++it)
//								{
//									TXTWriter << voxelID << "," << it->edgeID << "," << it->edgeLength << ",";
//									TXTWriter << helper::getNeuriteName(it->edgeLabel) << "," << it->connectedToSoma << ",";
//									TXTWriter << it->boutons << "," << it->spines << '\n';
//								}
//							}
//							else
//							{
//								std::cout << "ERROR! computeForDenseModel: Something is off. edgeInfoVector is empty!" << std::endl;
//							}
//						}
//
//						delete SpatialGraphsInsideBounds;
//					}
//					else
//					{
//						if (DEBUG)
//						{
//							std::cout << "No edges found in SpatialGraph" << std::endl;
//						}
//					}
//
//					if (DEBUG)
//					{
//						std::cout << "-------------------------" << std::endl;
//					}
//				}
//			}
//		}

//		std::cout << "[*] " << id+1 << "/" << cellIDList.size();
//		std::cout << " [" << cellTypeIDLabels[cellTypeIDs[cellID]];
//		std::cout << "] [Axon=" << helper::isAxonCellType(cellTypeIDs[cellID]) << "] ";
//
//		if (openedTxtWriter)
//		{
//			TXTWriter.close();
//			std::cout << ">> Written " << txtfilename << std::endl;
//		}
//		else
//		{
//			std::cout << "Skipped (" << cellID << ")" << std::endl;
//		}
//
//		delete spatialGraphReader;
//		delete sgOriginal;
//	}
}

/* Compute Neurite Snippets of Dense Model
 * Save them as .csv files */
void computeForDenseModel(std::vector<int> cellIDList, double origin[3], int dim[3], const char * outputpath)
{
	bool DEBUG = false;
	SBF = new BarrelField;

	double voxelSize = 50.0;
	const char * spatialGraphSetFilename =
			"/nas1/Data_regger/AXON_SAGA/Axon2/NeuroNet/cache_Vincent_complete_final/Realization20151007_ascii/Morphologies.am";
	std::vector< unsigned int > originalGraphIndices;
	std::vector< unsigned int > cellTypeIDs;
	std::vector< double * > spatialGraphTransforms;
	std::vector< std::string > originalGraphFiles;
	std::map< unsigned int, std::string > cellTypeIDLabels;
	Reader::readSpatialGraphSetFile(spatialGraphSetFilename, originalGraphIndices,
			cellTypeIDs, spatialGraphTransforms, originalGraphFiles, cellTypeIDLabels);

	std::cout << "Successfully read in SpatialGraphSet file." << std::endl;
	std::cout << "Processing individual spatial graphs now ... " << std::endl;
	std::cout << "Results are written in " << outputpath << std::endl;
	std::cout << "---------------------------------------------" << std::endl;

	for (int id = 0; id<cellIDList.size(); id++)
	{
		int cellID = cellIDList[id];
		int voxelID = 1;

		// Set up text file writer
		std::stringstream stream1;
		stream1 << cellID;
		std::string txtfilename = std::string(outputpath) + stream1.str() + "_snippets.csv";

		std::ofstream TXTWriter;
		bool openedTxtWriter = false;

		// Get Filename
		std::string originalName = originalGraphFiles[originalGraphIndices[cellID]];
		originalName = originalName.substr(1, originalName.size()-2);
		std::string loadName = helper::getRootFromPath(spatialGraphSetFilename) + originalName;

		// Load Spatial Graph
		Reader * spatialGraphReader = new Reader(loadName.c_str(), loadName.c_str());
		spatialGraphReader->readSpatialGraphFile(0);
		AmiraSpatialGraph * sgOriginal = spatialGraphReader->getSpatialGraph();

		// If axon, remove Dendrites
		if (helper::isAxonCellType(cellTypeIDs[cellID]))
		{
			sgOriginal->removeLabel(Dendrite);
			sgOriginal->removeLabel(ApicalDendrite);
			sgOriginal->removeLabel(BasalDendrite);
		}
		else // otherwise remove axon, and apply transformation
		{
			sgOriginal->removeLabel(Axon);

			// Apply Transformation
			sgOriginal->setTransformation(helper::amiraToVTKTransform(spatialGraphTransforms[cellID]));
			sgOriginal->applyTransformation();
		}

		if (DEBUG)
		{
			std::cout << "Read in " << loadName << std::endl;
			std::cout << "CellID = " << cellID << std::endl;
			std::cout << "CellType = " << cellTypeIDs[cellID];
			std::cout << " (" << cellTypeIDLabels[cellTypeIDs[cellID]] << ")" << std::endl;
			std::cout << "IsAxonCellType = " << helper::isAxonCellType(cellTypeIDs[cellID]) << std::endl;
		}

		// Go through all voxels
		for (int z = 0; z<dim[2]; z++)
		{
			for (int y = 0; y<dim[1]; y++)
			{
				for (int x = 0; x<dim[0]; x++, voxelID++)
				{
					double bounds[6] = {origin[0]+ x*voxelSize, origin[0]+ (x+1)*voxelSize,
							origin[1]+ y*voxelSize, origin[1]+ (y+1)*voxelSize,
							origin[2]+ z*voxelSize, origin[2]+ (z+1)*voxelSize};

					if (DEBUG)
					{
						std::cout << "-------------------------" << std::endl;
						std::cout << "Current bounds = [" << voxelID << "; ";
						std::cout << bounds[0] << " " << bounds[1] << ", ";
						std::cout << bounds[2] << " " << bounds[3] << ", ";
						std::cout << bounds[4] << " " << bounds[5] << "]" << std::endl;
						//std::cout << "#SpatialGraphEdges = "  << sgOriginal->getNumberOfEdges() << std::endl;
					}

					// Extract SpatialGraph Parts that are within Bounds
					bool sgWithinBounds = addIntersectionPointsToSG(sgOriginal,bounds);

					if (sgWithinBounds)
					{
						AmiraSpatialGraph * SpatialGraphsInsideBounds = getSpatialGraphsInsideBounds(sgOriginal,bounds,false);

						// Only write Spatial Graph if it contains at least one edge (otherwise it's empty)
						// Could be that there is an edge with only one point
						if (SpatialGraphsInsideBounds->getNumberOfEdges()>0)
						{
							double centerPt[3] = {bounds[0] + voxelSize/2, bounds[2] + voxelSize/2, bounds[4] + voxelSize/2};
							int laminarPos = SBF->laminarPosition(centerPt);

							std::vector< edgeInfo > edgeInfoVector = getStructureInsideBounds(SpatialGraphsInsideBounds,cellTypeIDs[cellID],laminarPos);
							if (edgeInfoVector.size() > 0)
							{
								if (!openedTxtWriter)
								{
									TXTWriter.open(txtfilename.c_str());
									if(TXTWriter.fail())
									{
										std::cout << "ERROR! computeForDenseModel: Writing " << txtfilename << " failed! " << std::endl;
										return;
									}
									TXTWriter << "voxelID,edgeID,edgeLength,edgeLabel,connectedToSoma,boutons,spines," << std::endl;
									openedTxtWriter = true;
								}

								// Print to file
								for (std::vector<edgeInfo>::iterator it=edgeInfoVector.begin(); it!=edgeInfoVector.end(); ++it)
								{
									TXTWriter << voxelID << "," << it->edgeID << "," << it->edgeLength << ",";
									TXTWriter << helper::getNeuriteName(it->edgeLabel) << "," << it->connectedToSoma << ",";
									TXTWriter << it->boutons << "," << it->spines << '\n';
								}
							}
							else
							{
								std::cout << "ERROR! computeForDenseModel: Something is off. edgeInfoVector is empty!" << std::endl;
							}
						}

						delete SpatialGraphsInsideBounds;
					}
					else
					{
						if (DEBUG)
						{
							std::cout << "No edges found in SpatialGraph" << std::endl;
						}
					}

					if (DEBUG)
					{
						std::cout << "-------------------------" << std::endl;
					}
				}
			}
		}

		std::cout << "[*] " << id+1 << "/" << cellIDList.size();
		std::cout << " [" << cellTypeIDLabels[cellTypeIDs[cellID]];
		std::cout << "] [Axon=" << helper::isAxonCellType(cellTypeIDs[cellID]) << "] ";

		if (openedTxtWriter)
		{
			TXTWriter.close();
			std::cout << ">> Written " << txtfilename << std::endl;
		}
		else
		{
			std::cout << "Skipped (" << cellID << ")" << std::endl;
		}

		delete spatialGraphReader;
		delete sgOriginal;
	}
}

/* Get Vector with CellIDs based on path/to/CellID.txt file */
std::vector<int> getCellIDList(const char * filename)
{
	std::vector<int> cellIDList;
	std::string currentLine;
	std::ifstream inputStream(filename);
	if(!inputStream.fail())
	{
		while(!inputStream.eof())
		{
			getline(inputStream,currentLine);
			if (currentLine.empty())
				break;

			cellIDList.push_back(atoi(currentLine.c_str()));
		}
		inputStream.close();
	}
	else
	{
		std::cout << "ERROR! getCellIDList: Could not find " << filename << std::endl;
	}

	return cellIDList;
}

/* Function to write EdgeInfo Vector to file */
void writeEdgeInfoVector(std::vector< edgeInfo > edgeInfoVector, const char * outputfilename)
{
	std::cout << "edgeID,edgeLength,edgeLabel,connectedToSoma," << std::endl;
	for (std::vector<edgeInfo>::iterator it=edgeInfoVector.begin(); it!=edgeInfoVector.end(); ++it)
	{
		std::cout << it->edgeID << "," << it->edgeLength << ",";
		std::cout << helper::getNeuriteName(it->edgeLabel) << "," << it->connectedToSoma << '\n';
	}
}

void printUsage()
{
	std::cout << "USAGE: ./GetCellPartsInBox [Inputfilename] [Outputfilename] [xmin] [xmax] [ymin] [ymax] [zmin] [zmax]" << std::endl;
	std::cout << "USAGE: ./GetCellPartsInBox [Inputfilename] [Outputfilename] [xmin] [xmax] [ymin] [ymax] [zmin] [zmax] [X]" << std::endl;
	std::cout << "USAGE: ./GetCellPartsInBox [Option: 0, 1, 2] (extract snippets of dendrites and axons innervating set voxels)" << std::endl;
}

// Is 3D Point on the Border of the bounds
bool IsPointOnBorder(double pt[3], double bounds[6])
{
	bool onBorder = (pt[X_COORD]==bounds[0] || pt[X_COORD]==bounds[1]
					   || pt[Y_COORD]==bounds[2] || pt[Y_COORD]==bounds[3]
					   || pt[Z_COORD]==bounds[4] || pt[Z_COORD]==bounds[5]);

	return onBorder;
}

std::vector< edgeInfo > getStructureInsideBounds(AmiraSpatialGraph * sg)
{
	// set CellTypeID to -1 (ignore)
	return getStructureInsideBounds(sg,-1);
}

/* Get Density Factor of Spine and Bouton Densities of given CellType*/
void getDensityFactors(int CellTypeID, std::map<int, double>& boutonDensity,
										std::map<int, double>& spineDensity)
{
	if (CellTypeID>-1)
	{
		std::map< unsigned int, std::map<int, double> >
							cellType2BoutonDensityMap = helper::getCellType2BoutonDensityMap();

		unsigned int preCellType;
		unsigned int postCellType;

		helper::getTypeIDs(CellTypeID, preCellType, postCellType);

		boutonDensity = cellType2BoutonDensityMap[preCellType];

		if (CellTypeID != VPM)
		{
			std::map< unsigned int, std::map<int, double> > cellType2SpineDensityMap
										= helper::getCellType2SpineDensityMap();

			spineDensity = cellType2SpineDensityMap[postCellType];
		}
	}
}

/* Updated Version that computes bouton density voxel wise and does not compute
 * intersection point between edges and laminar surfaces
 * Extract neurite snippets of spatial graph
 * CellTypeID used for bouton and PSTs scaling */
std::vector< edgeInfo > getStructureInsideBounds(AmiraSpatialGraph * sg, int CellTypeID, int laminarPosition)
{
	// Variables
	// Map with edgeID -> all point coordinates of this edge
	std::map<int, std::list< double* > > edgeIDtoPointCoordinates;
	std::map<int, int > edgeIDtoEdgeLabel;
	std::map<int, double > edgeIDtoEdgeLength;
	std::map<int, double > edgeIDtoBoutons;
	std::map<int, double > edgeIDtoSpines;

	int edgeIDSoma = -1;

	std::map<int, double> boutonDensity; // Supra,Gran,Infra
	std::map<int, double> spineDensity; // Apical,Basal
	getDensityFactors(CellTypeID,boutonDensity,spineDensity);

	bool DEBUG = false;

	if (DEBUG)
	{
		std::cout << "-------------------------------------" << std::endl;
		std::cout << "edgeID,edgeLength,edgeLabel,edgeNumOfPts," << std::endl;
	}

	// Go through Edges and fill maps
	// - edgeIDtoEdgeLabel
	// - edgeIDtoPointCoordinates
	// - edgeIDtoEdgeLength
	// - edgeIDtoBoutons
	// - edgeIDtoSpines
	// Already merge soma edges here!
	int edgeID = 1;
	for(std::vector< Edge * >::iterator edgeIt = sg->edgesBegin();
			edgeIt != sg->edgesEnd(); ++edgeIt, ++edgeID)
	{
		if (DEBUG)
		{
			std::cout << edgeID << "," << (*edgeIt)->segmentLength() << ",";
			std::cout << helper::getNeuriteName((*edgeIt)->label) << ",";
			std::cout << (*edgeIt)->edgePointCoordinates.size() << "," << std::endl;
		}

		bool skip = false;

		// Check soma
		if ((*edgeIt)->label==Soma)
		{
			if (edgeIDSoma==-1)
			{
				edgeIDSoma = edgeID;
			}
			else 	// if there has already been a soma edge
			{ 		// merge soma edges
				edgeIDtoEdgeLength[edgeIDSoma] += (*edgeIt)->segmentLength();
				edgeIDtoPointCoordinates[edgeIDSoma].merge((*edgeIt)->edgePointCoordinates);
				skip = true;
			}
		}
		else if (edgeID>1)
		{
			// check if previous edge points of same neurites match
			for (std::map<int, std::list< double* > >::iterator it=edgeIDtoPointCoordinates.begin();
							it!=edgeIDtoPointCoordinates.end(); ++it)
			{
				// check if edge labels match
				if ((*edgeIt)->label==edgeIDtoEdgeLabel[it->first])
				{
					// if they match, merge them
					if (isPtInBothLists(it->second, (*edgeIt)->edgePointCoordinates))
					{
						q[it->first] += (*edgeIt)->segmentLength();

						// Add Spine and Bouton Counts
						if ((*edgeIt)->label == Axon && boutonDensity.size()>0)
						{
							edgeIDtoBoutons[it->first] += (*edgeIt)->segmentLength()*boutonDensity[laminarPosition];
						}
						else if (((*edgeIt)->label == ApicalDendrite || (*edgeIt)->label == BasalDendrite)
								&& (spineDensity.size()>0))
						{
							edgeIDtoSpines[it->first] += (*edgeIt)->segmentLength() * spineDensity[(*edgeIt)->label];
						}

						edgeIDtoPointCoordinates[it->first].merge((*edgeIt)->edgePointCoordinates);

						skip = true;
						break;
					}
				}
			}
		}

		if (!skip)
		{
			edgeIDtoPointCoordinates[edgeID] = (*edgeIt)->edgePointCoordinates;
			edgeIDtoEdgeLabel[edgeID] = (*edgeIt)->label;
			edgeIDtoEdgeLength[edgeID] = (*edgeIt)->segmentLength();

			// Add Spine and Bouton Counts
			if ((*edgeIt)->label == Axon && boutonDensity.size()>0)
			{
				edgeIDtoBoutons[edgeID] = (*edgeIt)->segmentLength()*boutonDensity[laminarPosition];
			}
			else if (((*edgeIt)->label == ApicalDendrite || (*edgeIt)->label == BasalDendrite)
					&& (spineDensity.size()>0))
			{
				edgeIDtoSpines[edgeID] = edgeIDtoEdgeLength[edgeID] * spineDensity[(*edgeIt)->label];
			}
		}
	}

	// If soma exists, merge all neurites that are connected to the soma
	std::vector<int> edgeIDConnectedToSoma;

	if (edgeIDSoma!=-1)
	{
		if (DEBUG)
		{
			std::cout << "-------------------------------------" << std::endl;
			std::cout << "soma found in voxel!" << std::endl;
		}

		std::map<int,int> edgeLabelToEdgeIDSomaConnected;
		std::vector<int> edgeIDsForDeletion;

		for (std::map<int, std::list< double* > >::iterator it1=edgeIDtoPointCoordinates.begin();
				it1!=edgeIDtoPointCoordinates.end(); ++it1)
		{
			if (edgeIDtoEdgeLabel[it1->first]!=Soma)
			{
				// If connected to soma edge
				if (isPtInBothLists(it1->second, edgeIDtoPointCoordinates[edgeIDSoma]))
				{
					int edgeID = it1->first;
					int edgeLabel = edgeIDtoEdgeLabel[edgeID];

					// Check if label already exists
					if (edgeLabelToEdgeIDSomaConnected.count(edgeLabel)>0)
					{
						int edgeIDfather = edgeLabelToEdgeIDSomaConnected[edgeLabel];
						edgeIDtoEdgeLength[edgeIDfather] += edgeIDtoEdgeLength[edgeID];

						if (edgeIDtoBoutons.count(edgeIDfather)>0 &&
								edgeIDtoBoutons.count(edgeID)>0)
						{
							edgeIDtoBoutons[edgeIDfather] += edgeIDtoBoutons[edgeID];
						}
						if (edgeIDtoSpines.count(edgeIDfather)>0 &&
								edgeIDtoSpines.count(edgeID)>0)
						{
							edgeIDtoSpines[edgeIDfather] += edgeIDtoSpines[edgeID];
						}

						edgeIDsForDeletion.push_back(edgeID);
					}
					else // if not add it
					{
						edgeLabelToEdgeIDSomaConnected[edgeLabel] = edgeID;
						edgeIDConnectedToSoma.push_back(edgeID);
					}
				}
			}
		}

		for (int i = 0; i< edgeIDsForDeletion.size(); i++)
		{
			if (DEBUG)
			{
				std::cout << "Merging and deleting edgeID: " << edgeIDsForDeletion[i] << std::endl;
			}

			edgeIDtoPointCoordinates.erase(edgeIDsForDeletion[i]);
			edgeIDtoEdgeLabel.erase(edgeIDsForDeletion[i]);
			edgeIDtoEdgeLength.erase(edgeIDsForDeletion[i]);

			if (edgeIDtoSpines.count(edgeIDsForDeletion[i])>0)
			{
				edgeIDtoSpines.erase(edgeIDsForDeletion[i]);
			}

			if (edgeIDtoBoutons.count(edgeIDsForDeletion[i])>0)
			{
				edgeIDtoBoutons.erase(edgeIDsForDeletion[i]);
			}
		}
	}

	if (DEBUG)
	{
		std::cout << "-------------------------------------" << std::endl;
		std::cout << "edgeID,edgeLength,edgeLabel," << std::endl;

		for (std::map<int, std::list< double* > >::iterator it1=edgeIDtoPointCoordinates.begin();
				it1!=edgeIDtoPointCoordinates.end(); ++it1)
		{
			std::cout << it1->first << "," << edgeIDtoEdgeLength[it1->first];
			std::cout << "," << helper::getNeuriteName(edgeIDtoEdgeLabel[it1->first]) << "," << std::endl;
		}

		std::cout << "-------------------------------------" << std::endl;
	}

	std::vector< edgeInfo > edgeInfoVector;
	for (std::map<int, double>::iterator it=edgeIDtoEdgeLength.begin();
			it!=edgeIDtoEdgeLength.end(); ++it)
	{
		// EdgeLength should be larger than zero, otherwise skip it
		if (it->second > 0)
		{
			std::vector<int>::iterator itSoma =
					 find(edgeIDConnectedToSoma.begin(), edgeIDConnectedToSoma.end(), it->first);

			edgeInfo tmp = {.edgeID = it->first,
							.edgeLength = it->second,
							.edgeLabel = edgeIDtoEdgeLabel[it->first],
							.connectedToSoma = itSoma != edgeIDConnectedToSoma.end(),
							.boutons = -1,
							.spines = -1};

			if (edgeIDtoBoutons.count(it->first)>0)
				tmp.boutons = edgeIDtoBoutons[it->first];
			if (edgeIDtoSpines.count(it->first)>0)
				tmp.spines = edgeIDtoSpines[it->first];

			edgeInfoVector.push_back(tmp);
		}
	}

	return edgeInfoVector;
}


/* OLD VERSION that computes intersection with laminar surfaces
 * extract neurite snippets of spatial graph
 * CellTypeID used for bouton and PSTs scaling */
std::vector< edgeInfo > getStructureInsideBounds(AmiraSpatialGraph * sg, int CellTypeID)
{
	// Variables
	// Map with edgeID -> all point coordinates of this edge
	std::map<int, std::list< double* > > edgeIDtoPointCoordinates;
	std::map<int, int > edgeIDtoEdgeLabel;
	std::map<int, double > edgeIDtoEdgeLength;
	std::map<int, double > edgeIDtoBoutons;
	std::map<int, double > edgeIDtoSpines;

	int edgeIDSoma = -1;

	std::map<int, double> boutonDensity; // Supra,Gran,Infra
	std::map<int, double> spineDensity; // Apical,Basal
	getDensityFactors(CellTypeID,boutonDensity,spineDensity);

	bool DEBUG = false;

	if (DEBUG)
	{
		std::cout << "-------------------------------------" << std::endl;
		std::cout << "edgeID,edgeLength,edgeLabel,edgeNumOfPts," << std::endl;
	}

	// Go through Edges and fill maps
	// - edgeIDtoEdgeLabel
	// - edgeIDtoPointCoordinates
	// - edgeIDtoEdgeLength
	// - edgeIDtoBoutons
	// - edgeIDtoSpines
	// Already merge soma edges here!
	int edgeID = 1;
	for(std::vector< Edge * >::iterator edgeIt = sg->edgesBegin();
			edgeIt != sg->edgesEnd(); ++edgeIt, ++edgeID)
	{
		if (DEBUG)
		{
			std::cout << edgeID << "," << (*edgeIt)->segmentLength() << ",";
			std::cout << helper::getNeuriteName((*edgeIt)->label) << ",";
			std::cout << (*edgeIt)->edgePointCoordinates.size() << "," << std::endl;
		}

		bool skip = false;

		// Check soma
		if ((*edgeIt)->label==Soma)
		{
			if (edgeIDSoma==-1)
			{
				edgeIDSoma = edgeID;
			}
			else 	// if there has already been a soma edge
			{ 		// merge soma edges
				edgeIDtoEdgeLength[edgeIDSoma] += (*edgeIt)->segmentLength();
				edgeIDtoPointCoordinates[edgeIDSoma].merge((*edgeIt)->edgePointCoordinates);
				skip = true;
			}
		}
		else if (edgeID>1)
		{
			// check if previous edge points of same neurites match
			for (std::map<int, std::list< double* > >::iterator it=edgeIDtoPointCoordinates.begin();
							it!=edgeIDtoPointCoordinates.end(); ++it)
			{
				// check if edge labels match
				if ((*edgeIt)->label==edgeIDtoEdgeLabel[it->first])
				{
					// if they match, merge them
					if (isPtInBothLists(it->second, (*edgeIt)->edgePointCoordinates))
					{
						edgeIDtoEdgeLength[it->first] += (*edgeIt)->segmentLength();

						// Add Spine and Bouton Counts
						if ((*edgeIt)->label == Axon && boutonDensity.size()>0)
						{
							edgeIDtoBoutons[it->first] += computeBoutons((*edgeIt)->edgePointCoordinates,
															boutonDensity);
						}
						else if (((*edgeIt)->label == ApicalDendrite || (*edgeIt)->label == BasalDendrite)
								&& (spineDensity.size()>0))
						{
							edgeIDtoSpines[it->first] += (*edgeIt)->segmentLength() * spineDensity[(*edgeIt)->label];
						}

						edgeIDtoPointCoordinates[it->first].merge((*edgeIt)->edgePointCoordinates);

						skip = true;
						break;
					}
				}
			}
		}

		if (!skip)
		{
			edgeIDtoPointCoordinates[edgeID] = (*edgeIt)->edgePointCoordinates;
			edgeIDtoEdgeLabel[edgeID] = (*edgeIt)->label;
			edgeIDtoEdgeLength[edgeID] = (*edgeIt)->segmentLength();

			// Add Spine and Bouton Counts
			if ((*edgeIt)->label == Axon && boutonDensity.size()>0)
			{
				edgeIDtoBoutons[edgeID] = computeBoutons((*edgeIt)->edgePointCoordinates,
											boutonDensity);
			}
			else if (((*edgeIt)->label == ApicalDendrite || (*edgeIt)->label == BasalDendrite)
					&& (spineDensity.size()>0))
			{
				edgeIDtoSpines[edgeID] = edgeIDtoEdgeLength[edgeID] * spineDensity[(*edgeIt)->label];
			}
		}
	}

	// If soma exists, merge all neurites that are connected to the soma
	std::vector<int> edgeIDConnectedToSoma;

	if (edgeIDSoma!=-1)
	{
		if (DEBUG)
		{
			std::cout << "-------------------------------------" << std::endl;
			std::cout << "soma found in voxel!" << std::endl;
		}

		std::map<int,int> edgeLabelToEdgeIDSomaConnected;
		std::vector<int> edgeIDsForDeletion;

		for (std::map<int, std::list< double* > >::iterator it1=edgeIDtoPointCoordinates.begin();
				it1!=edgeIDtoPointCoordinates.end(); ++it1)
		{
			if (edgeIDtoEdgeLabel[it1->first]!=Soma)
			{
				// If connected to soma edge
				if (isPtInBothLists(it1->second, edgeIDtoPointCoordinates[edgeIDSoma]))
				{
					int edgeID = it1->first;
					int edgeLabel = edgeIDtoEdgeLabel[edgeID];

					// Check if label already exists
					if (edgeLabelToEdgeIDSomaConnected.count(edgeLabel)>0)
					{
						int edgeIDfather = edgeLabelToEdgeIDSomaConnected[edgeLabel];
						edgeIDtoEdgeLength[edgeIDfather] += edgeIDtoEdgeLength[edgeID];

						if (edgeIDtoBoutons.count(edgeIDfather)>0 &&
								edgeIDtoBoutons.count(edgeID)>0)
						{
							edgeIDtoBoutons[edgeIDfather] += edgeIDtoBoutons[edgeID];
						}
						if (edgeIDtoSpines.count(edgeIDfather)>0 &&
								edgeIDtoSpines.count(edgeID)>0)
						{
							edgeIDtoSpines[edgeIDfather] += edgeIDtoSpines[edgeID];
						}

						edgeIDsForDeletion.push_back(edgeID);
					}
					else // if not add it
					{
						edgeLabelToEdgeIDSomaConnected[edgeLabel] = edgeID;
						edgeIDConnectedToSoma.push_back(edgeID);
					}
				}
			}
		}

		for (int i = 0; i< edgeIDsForDeletion.size(); i++)
		{
			if (DEBUG)
			{
				std::cout << "Merging and deleting edgeID: " << edgeIDsForDeletion[i] << std::endl;
			}

			edgeIDtoPointCoordinates.erase(edgeIDsForDeletion[i]);
			edgeIDtoEdgeLabel.erase(edgeIDsForDeletion[i]);
			edgeIDtoEdgeLength.erase(edgeIDsForDeletion[i]);

			if (edgeIDtoSpines.count(edgeIDsForDeletion[i])>0)
			{
				edgeIDtoSpines.erase(edgeIDsForDeletion[i]);
			}

			if (edgeIDtoBoutons.count(edgeIDsForDeletion[i])>0)
			{
				edgeIDtoBoutons.erase(edgeIDsForDeletion[i]);
			}
		}
	}

	if (DEBUG)
	{
		std::cout << "-------------------------------------" << std::endl;
		std::cout << "edgeID,edgeLength,edgeLabel," << std::endl;

		for (std::map<int, std::list< double* > >::iterator it1=edgeIDtoPointCoordinates.begin();
				it1!=edgeIDtoPointCoordinates.end(); ++it1)
		{
			std::cout << it1->first << "," << edgeIDtoEdgeLength[it1->first];
			std::cout << "," << helper::getNeuriteName(edgeIDtoEdgeLabel[it1->first]) << "," << std::endl;
		}

		std::cout << "-------------------------------------" << std::endl;
	}

	std::vector< edgeInfo > edgeInfoVector;
	for (std::map<int, double>::iterator it=edgeIDtoEdgeLength.begin();
			it!=edgeIDtoEdgeLength.end(); ++it)
	{
		// EdgeLength should be larger than zero, otherwise skip it
		if (it->second > 0)
		{
			std::vector<int>::iterator itSoma =
					 find(edgeIDConnectedToSoma.begin(), edgeIDConnectedToSoma.end(), it->first);

			edgeInfo tmp = {.edgeID = it->first,
							.edgeLength = it->second,
							.edgeLabel = edgeIDtoEdgeLabel[it->first],
							.connectedToSoma = itSoma != edgeIDConnectedToSoma.end(),
							.boutons = -1,
							.spines = -1};

			if (edgeIDtoBoutons.count(it->first)>0)
				tmp.boutons = edgeIDtoBoutons[it->first];
			if (edgeIDtoSpines.count(it->first)>0)
				tmp.spines = edgeIDtoSpines[it->first];

			edgeInfoVector.push_back(tmp);
		}
	}

	return edgeInfoVector;
}

/* Compute Intersection point of two points cutting either L4 top or L4 bottom
 * Returns the intersectPt and the difference in length between
 * currPt, prevPt and intersectPt
 * If this length difference is large, switch the signWeigth (from 1 to -1)
 */
double computerIntersectionPt(int currLaminar, int prevLaminar, double intersectPt[3],
		double * currPt, double * prevPt, double signWeigth)
{
	double axis[3];
	for (int ii=0; ii<3; ii++)
	{
		axis[ii] =  signWeigth * (prevPt[ii] - currPt[ii]);
	}
	vtkMath::Normalize(axis);

	if (currLaminar==INFRA || prevLaminar==INFRA)
	{
		SBF->avgL4LowerSurface->intersectLine(axis,currPt);

		if(SBF->avgL4LowerSurface->isIntersectionFound())
		{
			SBF->avgL4LowerSurface->getLastIntersectPoint(intersectPt);
		}
		else
		{
			std::cout << "WARNING! computeBoutons: No Intersection Point Found" << std::endl;
		}
	}
	else if (currLaminar==SUPRA || prevLaminar==SUPRA)
	{
		SBF->avgL4UpperSurface->intersectLine(axis,currPt);

		if(SBF->avgL4UpperSurface->isIntersectionFound())
		{
			SBF->avgL4UpperSurface->getLastIntersectPoint(intersectPt);
		}
		else
		{
			std::cout << "WARNING! computeBoutons: No Intersection Point Found" << std::endl;
		}
	}
	else
	{
		std::cout << "WARNING! computeBoutons: Something is off!" << std::endl;
	}

	// Compute Boutons
	double lenPrev = helper::computeEuclideanDistance(prevPt, intersectPt);
	double lenCurr = helper::computeEuclideanDistance(currPt, intersectPt);
	double totalLen = helper::computeEuclideanDistance(currPt, prevPt);

	return fabs(lenPrev+lenCurr-totalLen);
}

/* Compute Boutons along edge points
 * returns the number of boutons
 * OLD VERSION: Computes boutons based on intersection with surface */
double computeBoutons(std::list< double* > pts, std::map<int, double> boutonDensity)
{
	bool DEBUG = false;

	std::list< double * >::iterator ptIt = pts.begin();
	int prevLaminar = SBF->laminarPosition(*ptIt);
	double * prevPt = (*ptIt);
	ptIt++;

	double boutons = 0.0;

	// Iterate through all points and get their laminar position
	for(; ptIt != pts.end(); ++ptIt)
	{
		double * currPt = (*ptIt);
		int currLaminar = SBF->laminarPosition(currPt);

		// If different layers and bouton densities between layers
		if ((currLaminar != prevLaminar) &&
				(boutonDensity[currLaminar] != boutonDensity[prevLaminar]))
		{
			// The direction of the axis matters and depends on whether exiting or entering surface
			// to make it easy, here I double check the length between currPt, prevPt and intersectPt
			// If the length does not match, the sign is changed to -1 and new intersectPt is computed
			double intersectPt[3];
			double diff = computerIntersectionPt(currLaminar, prevLaminar, intersectPt, currPt, prevPt, 1.0);
			if (diff>0.01)
			{
				diff = computerIntersectionPt(currLaminar, prevLaminar, intersectPt, currPt, prevPt, -1.0);
			}

			double lenPrev = helper::computeEuclideanDistance(prevPt, intersectPt);
			double lenCurr = helper::computeEuclideanDistance(currPt, intersectPt);
			double totalLen = helper::computeEuclideanDistance(currPt, prevPt);

			if (diff>0.01)
			{
				std::cout << "-----------------------" << std::endl;
				std::cout << "WARNING! computeBoutons: lengths do not match!" << std::endl;
				std::cout << "  Off by: " << diff << std::endl;
				std::cout << "  Length to/from intPt: " << lenPrev << " " << lenCurr << std::endl;
				std::cout << "  Total Length: " << totalLen << std::endl;
				std::cout << "  prevPt: " << prevPt[0] << " " << prevPt[1] << " " << prevPt[2] << std::endl;
				std::cout << "  currPt: " << currPt[0] << " " << currPt[1] << " " << currPt[2] << std::endl;
				std::cout << "  intPt: " << intersectPt[0] << " " << intersectPt[1] << " ";
				std::cout << intersectPt[2] << std::endl;
			}

			boutons = boutons + lenPrev*boutonDensity[prevLaminar] + lenCurr*boutonDensity[currLaminar];

			if (DEBUG)
			{
				std::cout << "-----------------------" << std::endl;
				std::cout << "prevPt: " << prevPt[0] << " " << prevPt[1] << " " << prevPt[2] << std::endl;
				std::cout << "currPt: " << currPt[0] << " " << currPt[1] << " " << currPt[2] << std::endl;
				std::cout << "intPt: " << intersectPt[0] << " " << intersectPt[1] << " ";
				std::cout << intersectPt[2] << std::endl;
				std::cout << "Length: " << lenPrev << " " << lenCurr << std::endl;
				std::cout << "Laminar: " << currLaminar << " " << prevLaminar << std::endl;
				std::cout << "BoutonDensity: " << boutonDensity[prevLaminar] << " ";
				std::cout << boutonDensity[currLaminar] << std::endl;
				std::cout << "Boutons: " << lenPrev*boutonDensity[prevLaminar] << " ";
				std::cout << lenCurr*boutonDensity[currLaminar] << std::endl;
				std::cout << "Total Boutons: " << boutons << std::endl;
			}
		}
		else // Same layer or same bouton density
		{
			double len = helper::computeEuclideanDistance(prevPt, currPt);
			boutons = boutons + len*boutonDensity[currLaminar];

			if (DEBUG)
			{
				/*std::cout << "-----------------------" << std::endl;
				std::cout << "prevPt: " << prevPt[0] << " " << prevPt[1] << " " << prevPt[2] << std::endl;
				std::cout << "currPt: " << currPt[0] << " " << currPt[1] << " " << currPt[2] << std::endl;
				std::cout << "Length: " << len << std::endl;
				std::cout << "Laminar: " << currLaminar << std::endl;
				std::cout << "BoutonDensity: " << boutonDensity[currLaminar] << std::endl;
				std::cout << "Boutons: " << len*boutonDensity[currLaminar] << std::endl;
				std::cout << "Total Boutons: " << boutons << std::endl;*/
			}
		}

		prevLaminar = currLaminar;
		prevPt = (*ptIt);
	}

	return boutons;
}

// Checks whether to lists have identical point
// identical: dist between points less than 0.01
// Used to find whether points of two edges are connected
bool isPtInBothLists(std::list< double* > ptList1, std::list< double* >ptList2)
{
	double epsilon = 0.01;

	// iterate over list1
	for (std::list< double* >::iterator pt1=ptList1.begin();
			pt1!=ptList1.end(); ++pt1)
	{
		// iterate over list2
		for (std::list< double* >::iterator pt2=ptList2.begin();
				pt2!=ptList2.end(); ++pt2)
		{
			double dist = helper::computeEuclideanDistance((*pt1),(*pt2));

			// Found edge point that is identical to edge point of previous edge
			if (dist<epsilon)
			{
				return true;
			}
		}
	}
	return false;
}

AmiraSpatialGraph * getSpatialGraphsInsideBounds(AmiraSpatialGraph * sg, double bounds[6])
{
	return getSpatialGraphsInsideBounds(sg, bounds, true);
}

// Extract SpatialGraph Edges that are within the given boundingbox
AmiraSpatialGraph * getSpatialGraphsInsideBounds(AmiraSpatialGraph * sg,
							double bounds[6], bool printMessages)
{
	AmiraSpatialGraph * SpatialGraphInsideBounds = new AmiraSpatialGraph;

    // Go through Edges
	for(std::vector< Edge * >::iterator edgeIt = sg->edgesBegin();
			edgeIt != sg->edgesEnd(); ++edgeIt)
	{
		std::list< double >::iterator edgeRadiusListIt = (*edgeIt)->radiusList.begin();
		double currentRad;
		double * currentPt;

		bool PtInBounds = false;

		// New List of EdgePoints and Radii
		std::list< double * > newEdgePointCoordinates;
		std::list< double > newRadList;

		// Go through all EdgePoints
		for(std::list< double * >::iterator edgeListIt =
				(*edgeIt)->edgePointCoordinates.begin();
				edgeListIt != (*edgeIt)->edgePointCoordinates.end();
				++edgeListIt, ++edgeRadiusListIt)
		{
			currentPt = *edgeListIt;
			currentRad = *edgeRadiusListIt;

			if (helper::isPtWithinBounds(currentPt,bounds))
			{
				// Add radius and EdgePoint List to new Edge
				newEdgePointCoordinates.push_back(currentPt);
				newRadList.push_back(currentRad);
				PtInBounds = true;
			}
			else if (!helper::isPtWithinBounds(currentPt,bounds) && PtInBounds)
			{ // Previous Point was within Bounds, but this one is not anymore
				// Add Edge to Spatial Graph
				addEdge(newEdgePointCoordinates,newRadList,
							(*edgeIt)->label,SpatialGraphInsideBounds);

				// Clear Point and Radii List
				newEdgePointCoordinates.clear();
				newRadList.clear();
				PtInBounds = false;
			}
		}

		// If entire Edge is within Bounds, add entire edge
		if (!newEdgePointCoordinates.empty())
		{
			// Create new Edge
			addEdge(newEdgePointCoordinates,newRadList,(*edgeIt)->label,SpatialGraphInsideBounds);

			// Clear Point and Radii List
			newEdgePointCoordinates.clear();
			newRadList.clear();
		}
	}

	// Count Cut Edges (Vertices on the border)
	int NumOfVerticesOnBounds = 0;
	for(std::vector< Vertex * >::iterator vertexIt = SpatialGraphInsideBounds->verticesBegin();
			vertexIt != SpatialGraphInsideBounds->verticesEnd(); ++vertexIt)
	{
		if (IsPointOnBorder((*vertexIt)->coordinates,bounds))
			NumOfVerticesOnBounds++;
	}

	if (printMessages)
	{
		std::cout << "SpatialGraphInsideBounds" << std::endl;
		std::cout << " #Edges Inside Bounds: " << SpatialGraphInsideBounds->getNumberOfEdges();
		std::cout << " #Cutting Sites: " << NumOfVerticesOnBounds << std::endl;
	}
	return SpatialGraphInsideBounds;
}

/* Add Edge and corresponding Vertices to SpatialGraph
 * Each Edge gets its own Vertex
 * NOTE: No edges added with less than 2 edge points */
void addEdge(std::list< double * > newEdgePointCoordinates,
		std::list< double > newRadList,	int edgeLabel, AmiraSpatialGraph * sg)
{
	if (newEdgePointCoordinates.size()<=1)
		return;

	// Create new Edge and add to SpatialGraph
	int newEdgeConnectivity[2];
	if(!sg->getNumberOfVertices())
	{
		newEdgeConnectivity[0] = 0;
		newEdgeConnectivity[1] = 1;
	}
	else
	{
		newEdgeConnectivity[0] = sg->getNumberOfVertices();
		newEdgeConnectivity[1] = sg->getNumberOfVertices()+1;
	}

	int newNumEdgePoints = newEdgePointCoordinates.size();
	Edge * newEdge = new Edge(newEdgeConnectivity, newNumEdgePoints, edgeLabel,
								newEdgePointCoordinates, newRadList);
	sg->addEdge(newEdge);

	// Create new Vertex and add to SpatialGraph
	double * startPoint = new double[3];
	double * endPoint = new double[3];
	startPoint = newEdgePointCoordinates.front();
	endPoint = newEdgePointCoordinates.back();
	Vertex * startVertex = new Vertex(startPoint, edgeLabel);
	Vertex * endVertex = new Vertex(endPoint, edgeLabel);

	sg->addVertex(startVertex);
	sg->addVertex(endVertex);
}

/* Adds IntersectionPoints between Edges and Bounds
 * this increases the number of EdgePoints
 * It enables easier cutting of edges along the bounds since no
 * interpolation is needed anymore
 * Uses vtkBox for computing intersection! */
bool addIntersectionPointsToSG(AmiraSpatialGraph * sg, double bounds[6])
{
	vtkBox * BBx = vtkBox::New();
	BBx->SetBounds(bounds);
	bool sgWithinBounds = false;

	for(std::vector< Edge * >::iterator edgeIt = sg->edgesBegin(); edgeIt != sg->edgesEnd(); ++edgeIt)
	{
		std::list< double * >::iterator edgeListIt;

		// Create Iterator, current one (+1) and previous one (0)
		edgeListIt = (*edgeIt)->edgePointCoordinates.begin();
		double * previousPt = *edgeListIt;
		++edgeListIt;

		// Interpolate Radius
		std::list< double >::iterator edgeRadiusListIt;
		edgeRadiusListIt = (*edgeIt)->radiusList.begin();
		double previousRad = *edgeRadiusListIt;
		++edgeRadiusListIt;

		if ((*edgeIt)->numEdgePoints==1)
		{
			std::cout << "  WARNING! Only One Edge Point Found! " << std::endl;
			std::cout << "    EdgePt: " << previousPt[X_COORD] << ", " << previousPt[Y_COORD];
			std::cout << ", " << previousPt[Z_COORD] << std::endl;
			std::cout << "    EdgeNo: " << (*edgeIt)->fatherID << std::endl;
		}

		bool radiusExist = true;
		if ((*edgeIt)->radiusList.empty() && radiusExist)
		{
			std::cout << "  WARNING! Radius List is empty!" << std::endl;
			radiusExist = false;
		}

		// Go through all points of each Edge starting with the second edge
		for(; edgeListIt != (*edgeIt)->edgePointCoordinates.end(); ++edgeListIt, ++edgeRadiusListIt)
		{
			double * currentPt = *edgeListIt;
			double currentRad = *edgeRadiusListIt;

			// Compute Intersection points
			double t1, t2, x1[3], x2[3];
			int plane1,plane2;
			int b = BBx->IntersectWithLine(bounds,previousPt,currentPt,t1,t2,x1,x2,plane1,plane2);

			// b is zero with line is totally outside of box
			// b is 1 if both pts are inside the box, if the intersect the box or
			// if they are on the edge of the box
			if (b != 0)
			{
				sgWithinBounds = true;

//				if ((t1>0 && t1<1) || (t2>0 && t2<1))
//				{
//					std::cout << "----------------------" << std::endl;
//					std::cout << "Edge in Box found ... " << std::endl;
//					std::cout << "  bounds = [" << bounds[0] << " " << bounds[1] << ", ";
//					std::cout << bounds[2] << " " << bounds[3] << ", ";
//					std::cout << bounds[4] << " " << bounds[5] << "]" << std::endl;
//					std::cout << "  p1 = [" << previousPt[0] << " " << previousPt[1] << " " << previousPt[2] << "]" << std::endl;
//					std::cout << "  p2 = [" << currentPt[0] << " " << currentPt[1] << " " << currentPt[2] << "]" << std::endl;
//				}

				if (t1>0 && t1<1)
				{
					double * pointerList = new double [3];
					for (int ii=0; ii<3; ii++)
						pointerList[ii] = x1[ii];

					// Add To Edge
					// Increment Number of Edge Points by 1
					(*edgeIt)->numEdgePoints += 1;
					// Insert Coordinates in List
					(*edgeIt)->edgePointCoordinates.insert(edgeListIt,pointerList);

					if (radiusExist)
					{
						double newRadius = currentRad + (previousRad - currentRad) * t1;
						(*edgeIt)->radiusList.insert(edgeRadiusListIt, newRadius);
					}

					//std::cout << "  x1 = [" << x1[0] << " " << x1[1] << " " << x1[2] << "] t1 = " << t1 << " p1 = " << plane1 << std::endl;
				}
				if (t2>0 && t2<1)
				{
					double * pointerList = new double [3];
					for (int ii=0; ii<3; ii++)
						pointerList[ii] = x2[ii];

					// Add To Edge
					// Increment Number of Edge Points by 1
					(*edgeIt)->numEdgePoints += 1;
					// Insert Coordinates in List
					(*edgeIt)->edgePointCoordinates.insert(edgeListIt,pointerList);

					if (radiusExist)
					{
						double newRadius = currentRad + (previousRad - currentRad) * t2;
						(*edgeIt)->radiusList.insert(edgeRadiusListIt, newRadius);
					}
					//std::cout << "  x2 = [" << x2[0] << " " << x2[1] << " " << x2[2] << "] t2 = " << t2 << " p2 = " << plane2 << std::endl;
				}
			}
			previousPt = currentPt;
			previousRad = currentRad;
		}
	}

	return sgWithinBounds;
}
