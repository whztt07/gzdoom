//
//---------------------------------------------------------------------------
//
// Copyright(C) 2018 Marisa Kirisame
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//--------------------------------------------------------------------------
//

#include "w_wad.h"
#include "cmdlib.h"
#include "r_data/models/models_ue1.h"

float unpackuvert( uint32_t n, int c )
{
	switch( c )
	{
	case 2:
		return ((int16_t)((n&0x7ff)<<5))/127.f;
	case 1:
		return ((int16_t)(((n>>11)&0x7ff)<<5))/127.f;
	case 0:
		return ((int16_t)(((n>>22)&0x3ff)<<6))/127.f;
	default:
		return 0.f;
	}
}

bool FUE1Model::Load( const char *filename, int lumpnum, const char *buffer, int length )
{
	mLumpNum = lumpnum;
	int lumpnum2;
	FMemLump lump2;
	const char *buffer2;
	FString realfilename = Wads.GetLumpFullName(lumpnum);
	if ( realfilename.IndexOf("_d.3d") == realfilename.Len()-5 )
	{
		realfilename.Substitute("_d.3d","_a.3d");
		lumpnum2 = Wads.CheckNumForFullName(realfilename);
		lump2 = Wads.ReadLump(lumpnum2);
		buffer2 = (char*)lump2.GetMem();
		// map structures
		dhead = (d3dhead*)(buffer);
		dpolys = (d3dpoly*)(buffer+sizeof(d3dhead));
		ahead = (a3dhead*)(buffer2);
		averts = (uint32_t*)(buffer2+sizeof(a3dhead));
	}
	else
	{
		realfilename.Substitute("_a.3d","_d.3d");
		lumpnum2 = Wads.CheckNumForFullName(realfilename);
		lump2 = Wads.ReadLump(lumpnum2);
		buffer2 = (char*)lump2.GetMem();
		// map structures
		dhead = (d3dhead*)(buffer2);
		dpolys = (d3dpoly*)(buffer2+sizeof(d3dhead));
		ahead = (a3dhead*)(buffer);
		averts = (uint32_t*)(buffer+sizeof(a3dhead));
	}
	// set counters
	numVerts = dhead->numverts;
	numFrames = ahead->numframes;
	numPolys = dhead->numpolys;
	numGroups = 0;
	groupIndices.Reset();
	uint8_t used[256] = {0};
	for ( int i=0; i<numPolys; i++ )
		used[dpolys[i].texnum] = 1;
	for ( int i=0; i<256; i++ )
	{
		if ( !used[i] ) continue;
		groupIndices.Push(i);
		numGroups++;
	}
	LoadGeometry();
	return true;
}

void FUE1Model::LoadGeometry()
{
	// populate vertex arrays
	for ( int i=0; i<numFrames; i++ )
	{
		for ( int j=0; j<numVerts; j++ )
		{
			UE1Vertex Vert;
			// unpack position
			Vert.Pos.X = unpackuvert(averts[j+i*numVerts],0);
			Vert.Pos.Y = unpackuvert(averts[j+i*numVerts],1);
			Vert.Pos.Z = unpackuvert(averts[j+i*numVerts],2);
			// push vertex (without normals, will be calculated later)
			verts.Push(Vert);
		}
	}
	// populate poly arrays
	for ( int i=0; i<numPolys; i++ )
	{
		UE1Poly Poly;
		// set indices
		for ( int j=0; j<3; j++ )
			Poly.V[j] = dpolys[i].vertices[j];
		// unpack coords
		for ( int j=0; j<3; j++ )
		{
			Poly.C[j].S = dpolys[i].uv[j][0]/255.f;
			Poly.C[j].T = dpolys[i].uv[j][1]/255.f;
		}
		Poly.texNum = dpolys[i].texnum;
		// push
		polys.Push(Poly);
	}
	// compute normals for vertex arrays
	// iterates through all polys and compute the average of all facet normals
	// from those who use this vertex. not pretty, but does the job
	for ( int i=0; i<numFrames; i++ )
	{
		for ( int j=0; j<numVerts; j++ )
		{
			UE1Vector nsum = {0,0,0};
			float total = 0.;
			for ( int k=0; k<numPolys; k++ )
			{
				if ( (polys[k].V[0] != j) && (polys[k].V[1] != j) && (polys[k].V[2] != j) ) continue;
				UE1Vector vert[3], dir[2], norm;
				// compute facet normal
				for ( int l=0; l<3; l++ )
					vert[l] = verts[polys[k].V[l]+numVerts*i].Pos;
				dir[0].X = vert[1].X-vert[0].X;
				dir[0].Y = vert[1].Y-vert[0].Y;
				dir[0].Z = vert[1].Z-vert[0].Z;
				dir[1].X = vert[2].X-vert[0].X;
				dir[1].Y = vert[2].Y-vert[0].Y;
				dir[1].Z = vert[2].Z-vert[0].Z;
				norm.X = dir[0].Y*dir[1].Z-dir[0].Z*dir[1].Y;
				norm.Y = dir[0].Z*dir[1].X-dir[0].X*dir[1].Z;
				norm.Z = dir[0].X*dir[1].Y-dir[0].Y*dir[1].X;
				float s = (float)sqrt(norm.X*norm.X+norm.Y*norm.Y+norm.Z*norm.Z);
				if ( s != 0.f )
				{
					norm.X /= s;
					norm.Y /= s;
					norm.Z /= s;
				}
				nsum.X += norm.X;
				nsum.Y += norm.Y;
				nsum.Z += norm.Z;
				total+=1.f;
			}
			verts[j+numVerts*i].Normal.X = nsum.X/total;
			verts[j+numVerts*i].Normal.Y = nsum.Y/total;
			verts[j+numVerts*i].Normal.Z = nsum.Z/total;
		}
	}
	// populate skin groups
	for ( int i=0; i<numGroups; i++ )
	{
		UE1Group Group;
		Group.numPolys = 0;
		for ( int j=0; j<numPolys; j++ )
		{
			if ( polys[j].texNum != groupIndices[i] )
				continue;
			Group.P.Push(j);
			Group.numPolys++;
		}
		groups.Push(Group);
	}
	// ... and it's finally done
}

void FUE1Model::UnloadGeometry()
{
	verts.Reset();
	polys.Reset();
	for ( int i=0; i<numGroups; i++ )
		groups[i].P.Reset();
	groups.Reset();
}

int FUE1Model::FindFrame( const char *name )
{
	// unsupported, there are no named frames
	return -1;
}

void FUE1Model::RenderFrame( FModelRenderer *renderer, FTexture *skin, int frame, int frame2, double inter, int translation )
{
	// the moment of magic
	if ( (frame >= numFrames) || (frame2 >= numFrames) ) return;
	renderer->SetInterpolation(inter);
	int vsize, fsize = 0, vofs = 0;
	for ( int i=0; i<numGroups; i++ ) fsize += groups[i].numPolys*3;
	for ( int i=0; i<numGroups; i++ )
	{
		vsize = groups[i].numPolys*3;
		FTexture *sskin = skin;
		if ( !sskin )
		{
			if ( curSpriteMDLFrame->surfaceskinIDs[curMDLIndex][i].isValid() )
				sskin = TexMan(curSpriteMDLFrame->surfaceskinIDs[curMDLIndex][i]);
			if ( !sskin )
			{
				vofs += vsize;
				continue;
			}
		}
		renderer->SetMaterial(sskin,false,translation);
		mVBuf->SetupFrame(renderer,vofs+frame*fsize,vofs+frame2*fsize,vsize);
		renderer->DrawArrays(0,vsize);
		vofs += vsize;
	}
	renderer->SetInterpolation(0.f);
}

void FUE1Model::BuildVertexBuffer( FModelRenderer *renderer )
{
	if ( mVBuf != NULL )
		return;
	int vsize = 0;
	for ( int i=0; i<numGroups; i++ )
		vsize += groups[i].numPolys*3;
	vsize *= numFrames;
	mVBuf = renderer->CreateVertexBuffer(false,numFrames==1);
	FModelVertex *vptr = mVBuf->LockVertexBuffer(vsize);
	int vidx = 0;
	for ( int i=0; i<numFrames; i++ )
	{
		for ( int j=0; j<numGroups; j++ )
		{
			for ( int k=0; k<groups[j].numPolys; k++ )
			{
				for ( int l=0; l<3; l++ )
				{
					UE1Vertex V = verts[polys[groups[j].P[k]].V[l]+i*numVerts];
					UE1Coord C = polys[groups[j].P[k]].C[l];
					FModelVertex *vert = &vptr[vidx++];
					vert->Set(V.Pos.X,V.Pos.Y,V.Pos.Z,C.S,C.T);
					vert->SetNormal(V.Normal.X,V.Normal.Y,V.Normal.Z);
				}
			}
		}
	}
	mVBuf->UnlockVertexBuffer();
}

void FUE1Model::AddSkins( uint8_t *hitlist )
{
	for ( int i=0; i<numGroups; i++ )
		if ( curSpriteMDLFrame->surfaceskinIDs[curMDLIndex][i].isValid() )
			hitlist[curSpriteMDLFrame->surfaceskinIDs[curMDLIndex][i].GetIndex()] |= FTextureManager::HIT_Flat;
}

FUE1Model::~FUE1Model()
{
	UnloadGeometry();
}
