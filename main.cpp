#include <stdio.h>
#include "Mesh.h"
#include "Simplify.h"

using namespace std;

int main(int argc, char* argv[])
{
    TriMesh mesh;
    int tarNum;
    argc = 4;
    mesh.loadMeshFromObjFile(argv[1]);
    sscanf(argv[3], "%d", &tarNum);
    AlgSimplify alg;
    alg.init(&mesh);
    alg.filter();
    alg.getQ();
    while (mesh.FaceSet().size() > tarNum)
    {
        alg.contractOne();
    }
    mesh.saveMeshToObjFile(argv[2]);
    puts("Successfully saved.");
    return 0;
}