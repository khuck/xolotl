#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Regression

#include <boost/test/included/unit_test.hpp>
#include <Diffusion1DHandler.h>
#include <HDF5NetworkLoader.h>
#include <XolotlConfig.h>
#include <DummyHandlerRegistry.h>
#include <mpi.h>

using namespace std;
using namespace xolotlCore;

/**
 * This suite is responsible for testing the Diffusion1DHandler.
 */
BOOST_AUTO_TEST_SUITE(Diffusion1DHandler_testSuite)

/**
 * Method checking the initialization of the off-diagonal part of the Jacobian,
 * and the compute diffusion methods.
 */
BOOST_AUTO_TEST_CASE(checkDiffusion) {
	// Initialize MPI for HDF5
	int argc = 0;
	char **argv;
	MPI_Init(&argc, &argv);

	// Create the network loader
	HDF5NetworkLoader loader =
			HDF5NetworkLoader(make_shared<xolotlPerf::DummyHandlerRegistry>());
	// Define the filename to load the network from
	string sourceDir(XolotlSourceDirectory);
	string pathToFile("/tests/testfiles/tungsten_diminutive.h5");
	string filename = sourceDir + pathToFile;
	// Give the filename to the network loader
	loader.setFilename(filename);

	// Load the network
	auto network = (PSIClusterReactionNetwork *) loader.load().get();
	// Get its size
	const int size = network->getAll()->size();

	// Create the diffusion handler
	Diffusion1DHandler diffusionHandler;

	// Create ofill
	int mat[size*size];
	int *ofill = &mat[0];

	// Initialize it
	diffusionHandler.initializeOFill(network, ofill);

	// All the clusters diffuse except the 7-th and 8-th one
	BOOST_REQUIRE_EQUAL(ofill[0], 1);
	BOOST_REQUIRE_EQUAL(ofill[10], 1);
	BOOST_REQUIRE_EQUAL(ofill[20], 1);
	BOOST_REQUIRE_EQUAL(ofill[30], 1);
	BOOST_REQUIRE_EQUAL(ofill[40], 1);
	BOOST_REQUIRE_EQUAL(ofill[50], 1);
	BOOST_REQUIRE_EQUAL(ofill[80], 1);

	// Check the total number of diffusing clusters
	BOOST_REQUIRE_EQUAL(diffusionHandler.getNumberOfDiffusing(), 7);

	// The size parameter in the x direction
	double sx = 1.0;

	// The arrays of concentration
	double concentration[3*size];
	double newConcentration[3*size];

	// Initialize their values
	for (int i = 0; i < 3*size; i++) {
		concentration[i] = (double) i * i;
		newConcentration[i] = 0.0;
	}

	// Set the temperature to 1000K to initialize the diffusion coefficients
	auto reactants = network->getAll();
	for (int i = 0; i < size; i++) {
		auto cluster = (PSICluster *) reactants->at(i);
		cluster->setTemperature(1000.0);
	}

	// Get pointers
	double *conc = &concentration[0];
	double *updatedConc = &newConcentration[0];

	// Get the offset for the grid point in the middle
	// Supposing the 3 grid points are laid-out as follow:
	// 0 | 1 | 2
	double *concOffset = conc + size;
	double *updatedConcOffset = updatedConc + size;

	// Fill the concVector with the pointer to the middle, left, and right grid points
	double **concVector = new double*[3];
	concVector[0] = concOffset; // middle
	concVector[1] = conc; // left
	concVector[2] = conc + 2 * size; // right

	// Compute the diffusion at this grid point
	diffusionHandler.computeDiffusion(network, concVector,
			updatedConcOffset, sx);

	// Check the new values of updatedConcOffset
	BOOST_REQUIRE_CLOSE(updatedConcOffset[0], 1.0393e+12, 0.01);
	BOOST_REQUIRE_CLOSE(updatedConcOffset[1], 5.0899e+11, 0.01);
	BOOST_REQUIRE_CLOSE(updatedConcOffset[2], 2.04785e+11, 0.01);
	BOOST_REQUIRE_CLOSE(updatedConcOffset[3], 2.70401e+11, 0.01);
	BOOST_REQUIRE_CLOSE(updatedConcOffset[4], 2.01239e+11, 0.01);
	BOOST_REQUIRE_CLOSE(updatedConcOffset[5], 4.98404e+09, 0.01);
	BOOST_REQUIRE_CLOSE(updatedConcOffset[6], 0.0, 0.01); // Does not diffuse
	BOOST_REQUIRE_CLOSE(updatedConcOffset[7], 0.0, 0.01); // Does not diffuse
	BOOST_REQUIRE_CLOSE(updatedConcOffset[8], 8.18607e+07, 0.01);

	// Initialize the indices and values to set in the Jacobian
	int nDiff = diffusionHandler.getNumberOfDiffusing();
	int indices[nDiff];
	double val[3*nDiff];
	// Get the pointer on them for the compute diffusion method
	int *indicesPointer = &indices[0];
	double *valPointer = &val[0];

	// Compute the partial derivatives for the diffusion a the grid point 1
	diffusionHandler.computePartialsForDiffusion(network, valPointer,
			indicesPointer, sx);

	// Check the values for the indices
	BOOST_REQUIRE_EQUAL(indices[0], 0);
	BOOST_REQUIRE_EQUAL(indices[1], 1);
	BOOST_REQUIRE_EQUAL(indices[2], 2);
	BOOST_REQUIRE_EQUAL(indices[3], 3);
	BOOST_REQUIRE_EQUAL(indices[4], 4);
	BOOST_REQUIRE_EQUAL(indices[5], 5);
	BOOST_REQUIRE_EQUAL(indices[6], 8);

	// Check some values
	BOOST_REQUIRE_CLOSE(val[1], 6.41544e+09, 0.01);
	BOOST_REQUIRE_CLOSE(val[4], 3.14191e+09, 0.01);
	BOOST_REQUIRE_CLOSE(val[5], 3.14191e+09, 0.01);
	BOOST_REQUIRE_CLOSE(val[6], -2.52821e+09, 0.01);
	BOOST_REQUIRE_CLOSE(val[9], -3.33828e+09, 0.01);

	// Finalize MPI
	MPI_Finalize();
}

BOOST_AUTO_TEST_SUITE_END()
