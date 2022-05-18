%~dp0/../Tool/glslc.exe Source/Culling.comp -o Spirv/Culling.comp.spv

%~dp0/../Tool/glslc.exe Source/BlueNoise.comp -o Spirv/BlueNoise.comp.spv

%~dp0/../Tool/glslc.exe Source/GBuffer.vert -o Spirv/GBuffer.vert.spv
%~dp0/../Tool/glslc.exe Source/GBuffer.frag -o Spirv/GBuffer.frag.spv

%~dp0/../Tool/glslc.exe Source/SSRIntersect.comp -o Spirv/SSRIntersect.comp.spv
%~dp0/../Tool/glslc.exe Source/SSRResolve.comp -o Spirv/SSRResolve.comp.spv
%~dp0/../Tool/glslc.exe Source/SSRTemporal.comp -o Spirv/SSRTemporal.comp.spv
%~dp0/../Tool/glslc.exe Source/SSRPrevInfoUpdate.comp -o Spirv/SSRPrevInfoUpdate.comp.spv

%~dp0/../Tool/glslc.exe Source/SSAOComputeAO.comp -o Spirv/SSAOComputeAO.comp.spv
%~dp0/../Tool/glslc.exe Source/SSAOSpatialFilterX.comp -o Spirv/SSAOSpatialFilterX.comp.spv
%~dp0/../Tool/glslc.exe Source/SSAOSpatialFilterY.comp -o Spirv/SSAOSpatialFilterY.comp.spv
%~dp0/../Tool/glslc.exe Source/SSAOTemporalFilter.comp -o Spirv/SSAOTemporalFilter.comp.spv

%~dp0/../Tool/glslc.exe Source/SSAOPrevInfoUpdate.comp -o Spirv/SSAOPrevInfoUpdate.comp.spv

%~dp0/../Tool/glslc.exe Source/FullScreen.vert -o Spirv/FullScreen.vert.spv

%~dp0/../Tool/glslc.exe Source/Lighting.frag -o Spirv/Lighting.frag.spv
%~dp0/../Tool/glslc.exe Source/Tonemapper.frag -o Spirv/Tonemapper.frag.spv

%~dp0/../Tool/glslc.exe Source/SDSMDepthMinMax.comp -o Spirv/SDSMDepthMinMax.comp.spv
%~dp0/../Tool/glslc.exe Source/SDSMShadowSetup.comp -o Spirv/SDSMShadowSetup.comp.spv
%~dp0/../Tool/glslc.exe Source/SDSMBuildDrawCall.comp -o Spirv/SDSMBuildDrawCall.comp.spv
%~dp0/../Tool/glslc.exe Source/SDSMDepth.vert -o Spirv/SDSMDepth.vert.spv
%~dp0/../Tool/glslc.exe Source/SDSMDepth.frag -o Spirv/SDSMDepth.frag.spv

%~dp0/../Tool/glslc.exe Source/HizBuild.comp -o Spirv/HizBuild.comp.spv

%~dp0/../Tool/glslc.exe Source/TAAMain.comp -o Spirv/TAAMain.comp.spv
%~dp0/../Tool/glslc.exe Source/TAASharpen.comp -o Spirv/TAASharpen.comp.spv

%~dp0/../Tool/glslc.exe Source/BloomDownsample.comp -o Spirv/BloomDownsample.comp.spv
%~dp0/../Tool/glslc.exe Source/BloomBlur.frag -o Spirv/BloomBlur.frag.spv
%~dp0/../Tool/glslc.exe Source/BloomComposite.frag -o Spirv/BloomComposite.frag.spv

%~dp0/../Tool/glslc.exe Source/BRDFLut.comp -o Spirv/BRDFLut.comp.spv
%~dp0/../Tool/glslc.exe Source/SpecularCaptureMipGen.comp -o Spirv/SpecularCaptureMipGen.comp.spv

%~dp0/../Tool/glslc.exe Source/AtmosphereMultipleScatteringLut.comp -o Spirv/AtmosphereMultipleScatteringLut.comp.spv
%~dp0/../Tool/glslc.exe Source/AtmospherePostProcessSky.comp -o Spirv/AtmospherePostProcessSky.comp.spv
%~dp0/../Tool/glslc.exe Source/AtmosphereSkyViewLut.comp -o Spirv/AtmosphereSkyViewLut.comp.spv
%~dp0/../Tool/glslc.exe Source/AtmosphereTransmittanceLut.comp -o Spirv/AtmosphereTransmittanceLut.comp.spv