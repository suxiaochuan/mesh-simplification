//
// Created by Haowen Xu on 9/28/16.
//

#include <set>
#include <iostream>
#include "Mesh.h"

void TriMesh::loadMeshFromObjFile(const char *strFileName)
{
    int v, n, t;
    double x, y, z;
    char buf[512];

    FILE* file;
    file = fopen(strFileName, "r");
    if (!file)
    {
        fprintf(stderr, "can't open data file \"%s\".\n", strFileName);
        exit(1);
    }
    vector<int> faces;
    char mtlName[512];
    int fCount = 0;
    vector<int> patch;
    while (fscanf(file, "%s", buf) != EOF)
    {
        switch (buf[0])
        {
            case '#':  /* comment */
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
            case 'v':  /* v, vn, vt */
                switch (buf[1])
                {
                    case '\0':  /* vertex */ // v x y z
                        /* eat up rest of line */
                        fscanf(file, "%lf %lf %lf", &x, &y, &z);
                        addVertex(Point3D(x, y, z));
                        break;
                    case 'n':  /* normal */  // vn x y z
                        /* eat up rest of line */
                        fscanf(file, "%lf %lf %lf", &x, &y, &z);
                        break;
                    case 't':  /* texcoord */  // vt x y
                        fscanf(file, "%lf %lf", &x, &y);
                        break;
                    default:
                        printf("_glmFirstPass(): Unknown token \"%s\".\n", buf);
                        exit(1);
                }
                break;
            case 'm':
                fgets(buf, sizeof(buf), file);
                sscanf(buf, "%s %s", buf, buf);
                break;
            case 'u':  // usemtl
                if (strcmp(buf, "usemtl") == 0)
                {
                    fscanf(file, "%s", mtlName);
                }
                else
                    fgets(buf, sizeof(buf), file);
                break;
            case 'g':  //group NOT SUPPORT
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
            case 'f': // face "f ia ib ic .."
                if (buf[1] == 0)
                {
                    v = n = t = 0;
                    faces.clear();

                    fscanf(file, "%s", buf);
                    if (strstr(buf, "//") != NULL)
                    {
                        // v//n
                        sscanf(buf, "%d//%d", &v, &n);
                        faces.push_back(v);

                        while (fscanf(file, "%d//%d", &v, &n) > 0)
                        {
                            faces.push_back(v);
                        }
                    }
                    else if (sscanf(buf, "%d/%d/%d", &v, &t, &n) == 3)
                    {
                        // v/t/n
                        faces.push_back(v);
                        while (fscanf(file, "%d/%d/%d", &v, &t, &n) > 0)
                        {
                            faces.push_back(v);
                        }
                    }
                    else if (sscanf(buf, "%d/%d", &v, &t) == 2)
                    {
                        // v/t
                        faces.push_back(v);
                        while (fscanf(file, "%d/%d", &v, &t) > 0)
                        {
                            faces.push_back(v);
                        }
                    }
                    else
                    {
                        // v
                        sscanf(buf, "%d", &v);
                        faces.push_back(v);
                        while (fscanf(file, "%d", &v) > 0)
                        {
                            faces.push_back(v);
                        }
                    }
                    addFace(faces[0], faces[1], faces[2]);
                    fCount++;
                }
                else if (buf[1] == 'p')
                {
                    patch.clear();
                    fscanf(file, "%d", &v);
                    while (fscanf(file, "%d", &v) == 1)
                    {
                        if(v != 0)
                        {
                            patch.push_back(v);
                        }
                    }
                    if(featurePatch == NULL)
                        featurePatch = new FeaturePatch();
                    featurePatch->addPatch(patch);
                }
                break;
            default:
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
        }
    }
    fclose(file);
    computeNormal();
}

void TriMesh::saveMeshToObjFile(const char* strFileName)
{
    FILE* file;
    file = fopen(strFileName, "w");
    if (!file)
    {
        fprintf(stderr, "can't open data file \"%s\".\n", strFileName);
        exit(1);
    }
    fprintf(file, "# LcMesh\n");
    int cnt = 1;
    unordered_map<int,int> mp;
    for (auto u = pointSet.begin(); u!=pointSet.end(); u++)
    {
        fprintf(file,"v %lf %lf %lf\n", u->second.pos.x, u->second.pos.y, u->second.pos.z);
        mp[u->first] = cnt++;
    }
    for (auto u = faceSet.begin(); u!=faceSet.end(); u++)
    {
        fprintf(file,"f %d %d %d\n", mp[u->second.index[0]], mp[u->second.index[1]], mp[u->second.index[2]]);
    }
    fclose(file);
    return ;
}

int TriMesh::addVertex(Point3D pos)
{
    Vertex newVertex(pos);
    pointSet[++maxPtIndex] = newVertex;
    return maxPtIndex;
}

int TriMesh::addFace(int a, int b, int c)
{
    Face newFace(a, b, c);
    faceSet[++maxFaceIndex] = newFace;
    for (int i=0; i<3; i++)
    {
        pointSet[newFace.index[i]].adjFaces.push_back(maxFaceIndex);
    }
    return maxFaceIndex;
}

bool TriMesh::deleteVertex(int index)
{
    for (int t=0; t<pointSet[index].adjFaces.size(); t++)
    {
        faceSet.erase(pointSet[index].adjFaces[t]);
    }
    pointSet.erase(index);
    return true;
}

bool TriMesh::deleteFace(int index)
{
    Face tmp = faceSet[index];
    for (int i = 0; i < 3; i++)
    {
        vector<int>::iterator itr = pointSet[tmp.index[i]].adjFaces.begin();
        while (itr != pointSet[tmp.index[i]].adjFaces.end())
        {
            if (*itr == index)
            {
                itr = pointSet[tmp.index[i]].adjFaces.erase(itr);
            }
            else
                itr++;
        }
    }
    faceSet.erase(index);
    if (featurePatch != NULL)
    {
        featurePatch->deleteFace(index);
    }
    return true;
}

bool TriMesh::buildTopo()
{
    return true;
}

bool TriMesh::contract(int a, int b)
{
    if (pointSet.find(a) == pointSet.end() || pointSet.find(b) == pointSet.end())
        return false;
    if (a == b) return false;

    set<int> st, st2;
    for (int i = 0; i < pointSet[a].adjFaces.size(); i++)
    {
        for (int j = 0; j < 3; j++)
            st.insert(faceSet[pointSet[a].adjFaces[i]].index[j]);
    }
    for (int i = 0; i < pointSet[b].adjFaces.size(); i++)
    {
        for (int j = 0; j < 3; j++)
            if (st.find(faceSet[pointSet[b].adjFaces[i]].index[j]) != st.end())
                st2.insert(faceSet[pointSet[b].adjFaces[i]].index[j]);
    }

    if (st2.size() > 4)
        return false;
    st.clear();

    for (int i = 0; i < pointSet[a].adjFaces.size(); i++)
    {
        Face &tmp = faceSet[pointSet[a].adjFaces[i]];
        int x[3];
        bool ok = true;
        for (int j = 0; j < 3; j++)
            if (tmp.index[j] == a)
                x[j] = b;
            else if (tmp.index[j] == b)
                ok = false;
            else
                x[j] = tmp.index[j];
        if (!ok)
            continue;

        Point3D newNormal = (pointSet[x[0]].pos - pointSet[x[1]].pos) ^ (pointSet[x[0]].pos - pointSet[x[2]].pos);
        if (newNormal.getLen() < 1e-8)
            continue;
        newNormal.normalize();
        if (newNormal * tmp.normal < -0.866)
            return false;
    }

    for (int i = 0; i < pointSet[a].adjFaces.size();i++)
    {
        st.insert(pointSet[a].adjFaces[i]);
    }
    for (int i = (int)pointSet[b].adjFaces.size() - 1; i >= 0; i--)
    {
        if (st.find(pointSet[b].adjFaces[i]) != st.end())
            deleteFace(pointSet[b].adjFaces[i]);
    }


    for (int i = 0; i < pointSet[a].adjFaces.size(); i++)
    {
        Face &tmp = faceSet[pointSet[a].adjFaces[i]];
        for (int j = 0; j < 3; j++)
            if (tmp.index[j] == a)
            {
                tmp.index[j] = b;
                pointSet[b].adjFaces.push_back(pointSet[a].adjFaces[i]);
            }
    }
    pointSet[a].adjFaces.clear();
    //pointSet[b].pos = (pointSet[b].pos +pointSet[a].pos)/2;
    deleteVertex(a);
    return true;
}

bool TriMesh::contract(int a, int b, Point3D ptNew)
{
    if (pointSet.find(a) == pointSet.end() || pointSet.find(b) == pointSet.end())
        return false;
    if (a == b) return false;

    set<int> st, st2;
    for (int i = 0; i < pointSet[a].adjFaces.size(); i++)
    {
        for (int j = 0; j < 3; j++)
            st.insert(faceSet[pointSet[a].adjFaces[i]].index[j]);
    }
    for (int i = 0; i < pointSet[b].adjFaces.size(); i++)
    {
        for (int j = 0; j < 3; j++)
            if (st.find(faceSet[pointSet[b].adjFaces[i]].index[j]) != st.end())
                st2.insert(faceSet[pointSet[b].adjFaces[i]].index[j]);
    }

    if (st2.size() > 4)
        return false;
    st.clear();

    for (int i = 0; i < pointSet[a].adjFaces.size(); i++)
    {
        Face &tmp = faceSet[pointSet[a].adjFaces[i]];
        Point3D x[3];
        bool ok = true;
        for (int j = 0; j < 3; j++)
            if (tmp.index[j] == a)
                x[j] = ptNew;
            else if (tmp.index[j] == b)
                ok = false;
            else
                x[j] = pointSet[tmp.index[j]].pos;
        if (!ok)
            continue;

        Point3D newNormal = (x[0] - x[1]) ^ (x[0] - x[2]);
        if (newNormal.getLen() < 1e-8)
            return false;
        newNormal.normalize();
        tmp.normal.normalize();
        if (newNormal * tmp.normal < -0.866)
        {
            return false;
        }
    }

    for (int i = 0; i < pointSet[b].adjFaces.size(); i++)
    {
        Face &tmp = faceSet[pointSet[b].adjFaces[i]];
        Point3D x[3];
        bool ok = true;
        for (int j = 0; j < 3; j++)
            if (tmp.index[j] == b)
                x[j] = ptNew;
            else if (tmp.index[j] == a)
                ok = false;
            else
                x[j] = pointSet[tmp.index[j]].pos;
        if(!ok)
            continue;

        Point3D newNormal = (x[0] - x[1]) ^ (x[0] - x[2]);
        if(newNormal.getLen() < 1e-8)
            //continue;
            return false;
        newNormal.normalize();
        tmp.normal.normalize();
        if (newNormal * tmp.normal < -0.866)
        {
            return false;
        }
    }

    for (int i = 0; i < pointSet[a].adjFaces.size(); i++)
    {
        st.insert(pointSet[a].adjFaces[i]);
    }
    for (int i = (int)pointSet[b].adjFaces.size() - 1; i >= 0; i--)
    {
        if (st.find(pointSet[b].adjFaces[i]) != st.end())
            deleteFace(pointSet[b].adjFaces[i]);
    }


    for (int i = 0; i < pointSet[a].adjFaces.size();i++)
    {
        Face &tmp = faceSet[pointSet[a].adjFaces[i]];
        for (int j = 0; j < 3; j++)
            if (tmp.index[j] == a)
            {
                tmp.index[j] = b;
                pointSet[b].adjFaces.push_back(pointSet[a].adjFaces[i]);
            }

    }
    pointSet[a].adjFaces.clear();
    //pointSet[b].pos = (pointSet[b].pos +pointSet[a].pos)/2;
    deleteVertex(a);
    pointSet[b].pos = ptNew;

    for (int i = 0; i < pointSet[b].adjFaces.size(); i++)
    {
        int faceId = pointSet[b].adjFaces[i];
        faceSet[faceId].normal = (pointSet[faceSet[faceId].index[0]].pos - pointSet[faceSet[faceId].index[1]].pos) ^ (pointSet[faceSet[faceId].index[0]].pos - pointSet[faceSet[faceId].index[2]].pos);
        faceSet[faceId].normal.normalize();
    }

    return true;
}

bool TriMesh::flip(int a, int b)
{
    set<int> commonFace;
    for (int i = 0; i < pointSet[a].adjFaces.size(); i++)
        commonFace.insert(pointSet[a].adjFaces[i]);
    int c = -1, d = -1;
    for (int i = 0; i < pointSet[b].adjFaces.size(); i++)
    {
        if (commonFace.find(pointSet[b].adjFaces[i]) != commonFace.end())
        {
            if (c == -1)
                c = pointSet[b].adjFaces[i];
            else
                d = pointSet[b].adjFaces[i];
        }
    }
    if (c == -1 || d == -1)
        return false;
    int newTri[2][3];
    for (int i = 0; i < 3; i++)
    {
        if (faceSet[c].index[i] != a && faceSet[c].index[i] != b)
        {
            newTri[0][0] = faceSet[c].index[i];
            newTri[0][1] = faceSet[c].index[(i + 1) % 3];
            newTri[1][2] = faceSet[c].index[i];
        }
        if (faceSet[d].index[i] != a && faceSet[d].index[i] != b)
        {
            newTri[1][0] = faceSet[d].index[i];
            newTri[1][1] = faceSet[d].index[(i + 1) % 3];
            newTri[0][2] = faceSet[d].index[i];
        }
    }
    addFace(newTri[0][0], newTri[0][1], newTri[0][2]);
    addFace(newTri[1][0], newTri[1][1], newTri[1][2]);
    deleteFace(c);
    deleteFace(d);
    return true;
}

ADJSET TriMesh::getAdjVertexOfVertex(int u)
{
    ADJSET ret;
    for (int i = 0; i < pointSet[u].adjFaces.size(); i++)
    {
        for (int j = 0; j < 3; j++)
            ret.insert(faceSet[pointSet[u].adjFaces[i]].index[j]);
    }
    ret.erase(u);
    return ret;
}

vector<int> TriMesh::getAdjFaceOfEdge(int a,int b)
{
    vector<int> ret;
    unordered_map<int,int> face_mp;
    for (int i = 0; i < pointSet[a].adjFaces.size(); i++)
        face_mp[pointSet[a].adjFaces[i]] = 1;
    for (int i = 0; i < pointSet[b].adjFaces.size(); i++)
        if (face_mp[pointSet[b].adjFaces[i]] == 1)
            ret.push_back(pointSet[b].adjFaces[i]);
    return ret;
}

void TriMesh::computeNormal()
{
    for (auto t = faceSet.begin(); t != faceSet.end(); t++)
    {
        t->second.normal = (pointSet[t->second.index[0]].pos - pointSet[t->second.index[1]].pos) ^ (pointSet[t->second.index[0]].pos - pointSet[t->second.index[2]].pos);
        t->second.normal.normalize();
    }
}

bool TriMesh::constractFeatureGraph()
{
    if (featurePatch == nullptr)
        return false;
    for (auto v = pointSet.begin(); v != pointSet.end(); ++v)
    {
        set<int> colors;
        for (int i = 0; i < v->second.adjFaces.size(); i++)
            colors.insert(featurePatch->getColor(v->second.adjFaces[i]));
        colors.erase(-1);
        if (colors.size() > 1)
        {
            for (auto u1 = colors.begin(); u1 != colors.end(); ++u1)
                for (auto u2 = colors.begin(); u2 != u1; ++u2)
                    featurePatch->patchGraphEdges.insert((((long long int)(*u2)) << 32) + (*u1));
        }
    }
}

double TriMesh::getArea(int faceIndex)
{
    return ((pointSet[faceSet[faceIndex].index[0]].pos - pointSet[faceSet[faceIndex].index[1]].pos) ^ (pointSet[faceSet[faceIndex].index[0]].pos - pointSet[faceSet[faceIndex].index[2]].pos)).getLen() / 2;
}

bool TriMesh::isBoundaryEdge(int vertexIndex1, int vertexIndex2)
{
    unordered_map<int,int> face_mp;
    int cnt = 0;
    for (int i = 0; i < pointSet[vertexIndex1].adjFaces.size(); i++)
        face_mp[pointSet[vertexIndex1].adjFaces[i]] = 1;
    for (int i = 0; i < pointSet[vertexIndex2].adjFaces.size(); i++)
        if (face_mp[pointSet[vertexIndex2].adjFaces[i]] == 1)
            ++cnt;
    return cnt == 1;
}

bool TriMesh::isBoundaryVertex(int vertexIndex)
{
    unordered_map<int,int> vertex_mp;
    for (int i = 0; i<pointSet[vertexIndex].adjFaces.size(); i++)
    {
        for (int j=0; j < 3; j++)
        {
            ++vertex_mp[faceSet[pointSet[vertexIndex].adjFaces[i]].index[j]];
        }
    }
    for (int i = 0; i < pointSet[vertexIndex].adjFaces.size(); i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (vertex_mp[faceSet[pointSet[vertexIndex].adjFaces[i]].index[j]] == 1)
                return true;
        }
    }
    return false;
}