#version 460

#extension GL_GOOGLE_include_directive: require
#include "VoxelDimension.h"

struct SVoxelDraw
{
	vec3 Position;
	uint FirstInstance;
};

struct SDrawCommand
{
	uint IndexCount;
    uint InstanceCount;
    uint FirstIndex;
    uint VertexOffset;
    uint FirstInstance;
};

layout (push_constant) uniform PushConstants
{
	uint LateRender;

	uint ObjectsCount;

	uint MinMaxSampler;
};

layout (set = 0, binding = 0) uniform CameraBuffer
{
	mat4 View;
	mat4 Proj;
	mat4 ProjUnjittered;

	mat4 PrevView;
	mat4 PrevProj;
	
	vec4 CameraPosition; 
    vec4 Viewport; // Width, Height, Near, Far
	vec4 Frustum[6];
};

layout (set = 0, binding = 1) readonly buffer Draws
{
	SVoxelDraw Draw[];
};

layout (set = 0, binding = 2) writeonly buffer DrawCommands
{
	SDrawCommand DrawCommand[];
};

layout (set = 0, binding = 3) buffer DrawCounter
{
	uint DrawCount;
};

layout (set = 0, binding = 4) buffer Visibilities
{
	uint Visibility[];
};

layout (set = 0, binding = 5) uniform sampler2D HiDepthTexture;

vec3 ProjectPoint(vec3 Point)
{
	vec4 V = Proj * vec4(Point, 1.0);
	return V.xyz / V.w;
}

bool ProjectAABB(vec3 Points[8], float Near, out vec4 AABB)
{
    for (uint I = 0; I < 8; I++)
        if (Points[I].z > Near)
            return false;

    AABB.xy = vec2(1.0);
    AABB.zw = vec2(0.0);
    for (uint I = 0; I < 8; I++)
    {
        vec2 PointScreen = ProjectPoint(Points[I]).xy * vec2(0.5, -0.5) + vec2(0.5);
	    AABB.x = min(AABB.x, PointScreen.x);
	    AABB.y = min(AABB.y, PointScreen.y);
	    AABB.z = max(AABB.z, PointScreen.x);
	    AABB.w = max(AABB.w, PointScreen.y);
    }

    if ((AABB.x >= AABB.z) || (AABB.y >= AABB.w))
        return false;

	AABB = clamp(AABB, 0.0, 1.0);

    return true;
}

vec3 ClosestPtPointAABB(vec3 CameraPosition, vec4 Points[8])
{
    const float FloatMax = 3.402823466e+38;

    vec3 Min = vec3(FloatMax, FloatMax, FloatMax);
    vec3 Max = vec3(-FloatMax, -FloatMax, -FloatMax);
    for (uint I = 0; I < 8; I++)
    {
        Min = min(Min, Points[I].xyz);
        Max = max(Max, Points[I].xyz);
    }

    vec3 ClosestPt = clamp(CameraPosition, Min, Max);
    return ClosestPt;
}

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
void main()
{
	uint Index = gl_GlobalInvocationID.x;
	if((Index < ObjectsCount) && (Draw[Index].FirstInstance != 0xFFFFFFFF))
	{
		if ((LateRender == 0) && (Visibility[Index] == 0))
			return;

        vec4 Points[8];
        Points[0] = vec4(Draw[Index].Position, -1.0);
        Points[1] = vec4(Draw[Index].Position + VoxelDim*vec3(1.0, 0.0, 0.0), -1.0);
        Points[2] = vec4(Draw[Index].Position + VoxelDim*vec3(1.0, 0.0, 1.0), -1.0);
        Points[3] = vec4(Draw[Index].Position + VoxelDim*vec3(0.0, 0.0, 1.0), -1.0);
        Points[4] = vec4(Draw[Index].Position + VoxelDim*vec3(0.0, 1.0, 0.0), -1.0);
        Points[5] = vec4(Draw[Index].Position + VoxelDim*vec3(1.0, 1.0, 0.0), -1.0);
        Points[6] = vec4(Draw[Index].Position + VoxelDim*vec3(1.0, 1.0, 1.0), -1.0);
        Points[7] = vec4(Draw[Index].Position + VoxelDim*vec3(0.0, 1.0, 1.0), -1.0);

        // Frustum culling
		bool bVisible = true;
		for (uint I = 0; I < 6; I++)
        {
            bool bInsidePlane = false;
            for (uint J = 0; J < 8; J++)
            {
                bInsidePlane = bInsidePlane || (dot(Points[J], Frustum[I]) >= 0.0);
            }
			bVisible = bVisible && bInsidePlane;
        }

        // Occlusion culling
		if (bVisible && (LateRender == 1))
		{
			const float Near = Viewport.z;
			const float Far = Viewport.w;

            vec3 PointsVS[8];
            for (uint I = 0; I < 8; I++)
                PointsVS[I] = vec3(View * vec4(Points[I].xyz, 1.0));

			vec4 AABB;
			if (ProjectAABB(PointsVS, Near, AABB))
			{
				vec2 TextureSize = textureSize(HiDepthTexture, 0).xy;
				float Width = (AABB.z - AABB.x) * TextureSize.x;
				float Height = (AABB.w - AABB.y) * TextureSize.y;

				float Level = ceil(log2(max(Width, Height)));
				float MaxDepth;
				if (MinMaxSampler > 0)
				{
					// This computes max depth of 2x2 texel quad
					MaxDepth = textureLod(HiDepthTexture, 0.5*(AABB.xy + AABB.zw), Level).x;
				}
				else
				{
					vec2 MipSize = vec2(max(uint(TextureSize.x) >> uint(Level), 1), max(uint(TextureSize.y) >> uint(Level), 1));
					vec2 MipTexelSize = 1.0 / MipSize;

					vec2 Center = 0.5*(AABB.xy + AABB.zw);
					MaxDepth = textureLod(HiDepthTexture, Center + 0.51 * vec2(-MipTexelSize.x, -MipTexelSize.y), Level).x;
					MaxDepth = max(MaxDepth, textureLod(HiDepthTexture, Center + 0.51 * vec2(MipTexelSize.x, -MipTexelSize.y), Level).x);
					MaxDepth = max(MaxDepth, textureLod(HiDepthTexture, Center + 0.51 * vec2(-MipTexelSize.x, MipTexelSize.y), Level).x);
					MaxDepth = max(MaxDepth, textureLod(HiDepthTexture, Center + 0.51 * vec2(MipTexelSize.x, MipTexelSize.y), Level).x);
				}

				vec3 ClosestObjectP = ClosestPtPointAABB(CameraPosition.xyz, Points);
				float MinObjectDepth = length(ClosestObjectP - CameraPosition.xyz) / Far;

				bVisible = bVisible && (MinObjectDepth <= MaxDepth);
			}
		}

		if (bVisible && ((LateRender == 0) || (Visibility[Index] == 0)))
		{
			uint CommandIndex = atomicAdd(DrawCount, 1);

			DrawCommand[CommandIndex].IndexCount = 36;
			DrawCommand[CommandIndex].InstanceCount = 1;
			DrawCommand[CommandIndex].FirstIndex = 0;
			DrawCommand[CommandIndex].VertexOffset = 0;
			DrawCommand[CommandIndex].FirstInstance = Draw[Index].FirstInstance;
		}

		if (LateRender == 1)
		{
			Visibility[Index] = bVisible ? 1 : 0;
		}
	}
}