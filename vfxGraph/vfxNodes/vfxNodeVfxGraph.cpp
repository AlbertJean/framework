/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "tinyxml2.h"
#include "vfxGraph.h"
#include "vfxNodeOutput.h"
#include "vfxNodeVfxGraph.h"
#include <algorithm>

using namespace tinyxml2;

extern int VFXGRAPH_SX;
extern int VFXGRAPH_SY;

VFX_NODE_TYPE(VfxNodeVfxGraph)
{
	typeName = "vfxGraph";
	
	in("file", "string");
	out("image", "image");
}

VfxNodeVfxGraph::VfxNodeVfxGraph()
	: VfxNodeBase()
	, typeDefinitionLibrary(nullptr)
	, graph(nullptr)
	, vfxGraph(nullptr)
	, currentFilename()
	, imageOutput(nullptr)
{
	imageOutput = new VfxImage_Texture();
	
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Filename, kVfxPlugType_String);
	addOutput(kOutput_Image, kVfxPlugType_Image, imageOutput);
}

VfxNodeVfxGraph::~VfxNodeVfxGraph()
{
	close();
	
	delete imageOutput;
	imageOutput = nullptr;
}

void VfxNodeVfxGraph::open(const char * filename)
{
	close();
	
	//
	
	bool success = false;
	
	clearEditorIssue();
	
	XMLDocument d;
	
	if (d.LoadFile(filename) != XML_SUCCESS)
	{
		setEditorIssue("failed to open file");
	}
	else
	{
		graph = new Graph();
		
		typeDefinitionLibrary = new GraphEdit_TypeDefinitionLibrary();
		
		createVfxTypeDefinitionLibrary(*typeDefinitionLibrary, g_vfxEnumTypeRegistrationList, g_vfxNodeTypeRegistrationList);
		
		if (graph->loadXml(d.RootElement(), typeDefinitionLibrary) == false)
		{
			setEditorIssue("failed to parse XML");
		}
		else
		{
			auto restore = g_currentVfxGraph;
			g_currentVfxGraph = nullptr;
			{
				vfxGraph = constructVfxGraph(*graph, typeDefinitionLibrary);
			}
			g_currentVfxGraph = restore;
			
			if (vfxGraph == nullptr)
			{
				setEditorIssue("failed to create vfx graph");
			}
			else
			{
				success = true;
			}
		}
	}
	
	if (success == false)
	{
		close();
	}
}

void VfxNodeVfxGraph::close()
{
	setDynamicOutputs(nullptr, 0);
	
	auto restore = g_currentVfxGraph;
	g_currentVfxGraph = nullptr;
	{
		delete vfxGraph;
		vfxGraph = nullptr;
	}
	g_currentVfxGraph = restore;
	
	delete graph;
	graph = nullptr;
	
	delete typeDefinitionLibrary;
	typeDefinitionLibrary = nullptr;
	
	//
	
	imageOutput->texture = 0;
}

void VfxNodeVfxGraph::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeVfxGraph);
	
	if (isPassthrough)
	{
		currentFilename.clear();
		
		close();
		
		return;
	}
	
	const char * filename = getInputString(kInput_Filename, nullptr);
	
	if (filename == nullptr)
	{
		currentFilename.clear();
		
		close();
	}
	else if (filename != currentFilename)
	{
		currentFilename = filename;
		
		open(filename);
	}
	
	if (vfxGraph != nullptr)
	{
		auto restore = g_currentVfxGraph;
		g_currentVfxGraph = nullptr;
		{
			vfxGraph->tick(VFXGRAPH_SX, VFXGRAPH_SY, dt);
		}
		g_currentVfxGraph = restore;
		
		// update dynamic outputs
		
		std::vector<DynamicOutput> outputs;
		outputs.reserve(vfxGraph->outputNodes.size());
	
		for (auto outputNode : vfxGraph->outputNodes)
		{
			DynamicOutput output;
			output.name = outputNode->name;
			output.type = kVfxPlugType_Float;
			output.mem = &outputNode->value;
			
			outputs.push_back(output);
		}
		
		std::sort(outputs.begin(), outputs.end(), [](DynamicOutput & o1, DynamicOutput & o2) { return o1.name < o2.name; });
	
		setDynamicOutputs(&outputs[0], outputs.size());
	}
}

void VfxNodeVfxGraph::draw() const
{
	vfxCpuTimingBlock(VfxNodeVfxGraph);
	
	if (isPassthrough || vfxGraph == nullptr)
	{
		return;
	}
	
	auto restoreGraph = g_currentVfxGraph;
	auto restoreSurface = g_currentVfxSurface;
	g_currentVfxGraph = nullptr;
	g_currentVfxSurface = nullptr;
	{
		imageOutput->texture = vfxGraph->traverseDraw(VFXGRAPH_SX, VFXGRAPH_SY);
	}
	g_currentVfxGraph = restoreGraph;
	g_currentVfxSurface = restoreSurface;
}

void VfxNodeVfxGraph::init(const GraphNode & node)
{
	if (isPassthrough)
		return;
	
	const char * filename = getInputString(kInput_Filename, nullptr);
	
	if (filename != nullptr)
	{
		open(filename);
	}
}
