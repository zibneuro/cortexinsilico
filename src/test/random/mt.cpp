#include "RandomGenerator.h"
#include <QDebug>

void
printUsage()
{
    qDebug() << "Usage: ./mt <seed>";     
}

int
main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printUsage();
        return 1;
    }
        
    int seed = atoi(argv[1]);
    RandomGenerator::testMersenne(seed);    
    return 0;
}
