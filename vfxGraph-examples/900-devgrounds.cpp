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

#include "framework.h"
#include "graph.h"
#include "StringEx.h"

#include "vfxGraph.h"
#include "vfxGraphRealTimeConnection.h"
#include "vfxNodes/oscEndpointMgr.h"
#include "vfxTypes.h"

#include "mediaplayer/MPUtil.h"
#include "../libparticle/ui.h"
#include "Timer.h"
#include "tinyxml2.h"
#include "vfxNodeBase.h"

using namespace tinyxml2;

//#define FILENAME "graph.xml"
//#define FILENAME "kinect.xml"
//#define FILENAME "yuvtest.xml"
//#define FILENAME "timeline.xml"
//#define FILENAME "channels.xml"
//#define FILENAME "drawtest.xml"
//#define FILENAME "resourceTest.xml"
//#define FILENAME "drawImageTest.xml"
//#define FILENAME "oscpath.xml"
//#define FILENAME "oscpathlist.xml"
//#define FILENAME "wekinatorTest.xml"
//#define FILENAME "testVfxGraph.xml"
//#define FILENAME "sampleTest.xml"
//#define FILENAME "midiTest.xml"
//#define FILENAME "draw3dTest.xml"
//#define FILENAME "nodeDataTest.xml"
//#define FILENAME "fsfxv2Test.xml"
//#define FILENAME "kinectTest2.xml"
//#define FILENAME "testOscilloscope.xml"
//#define FILENAME "testVisualizers.xml"
//#define FILENAME "testRibbon3.xml"
#define FILENAME "testPetals.xml"

extern const int GFX_SX;
extern const int GFX_SY;

const int GFX_SX = 1024;
const int GFX_SY = 768;

//

struct VfxNodeResourceTest : VfxNodeBase
{
	enum Input
	{
		kInput_Randomize,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	float value;
	
	VfxNodeResourceTest()
		: VfxNodeBase()
		, value(0.f)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Randomize, kVfxPlugType_Trigger);
		addOutput(kOutput_Value, kVfxPlugType_Float, &value);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		const char * resourceData = node.getResource("float", "value", nullptr);
		
		if (resourceData != nullptr)
		{
			XMLDocument d;
			
			if (d.Parse(resourceData) == XML_SUCCESS)
			{
				auto e = d.FirstChildElement("float");
				
				if (e != nullptr)
				{
					value = e->FloatAttribute("value");
				}
			}
		}
	}
	
	virtual void handleTrigger(const int socketIndex) override
	{
		value = random(0.f, 1.f);
	}
	
	virtual void beforeSave(GraphNode & node) const override
	{
		if (value == 0.f)
		{
			node.clearResource("float", "value");
		}
		else
		{
			XMLPrinter p;
			
			p.OpenElement("float");
			{
				p.PushAttribute("value", value);
			}
			p.CloseElement();
			
			const char * resourceData = p.CStr();
			
			node.setResource("float", "value", resourceData);
		}
	}
};

VFX_NODE_TYPE(VfxNodeResourceTest)
{
	typeName = "test.resource";
	
	resourceTypeName = "timeline";
	
	in("randomize!", "trigger");
	out("value", "float");
}

//

static VfxPlugType stringToVfxPlugType(const std::string & typeName)
{
	VfxPlugType type = kVfxPlugType_None;

	if (typeName == "bool")
		type = kVfxPlugType_Bool;
	else if (typeName == "int")
		type = kVfxPlugType_Int;
	else if (typeName == "float")
		type = kVfxPlugType_Float;
	else if (typeName == "string")
		type = kVfxPlugType_String;
	else if (typeName == "channel")
		type = kVfxPlugType_Channel;
	else if (typeName == "color")
		type = kVfxPlugType_Color;
	else if (typeName == "image")
		type = kVfxPlugType_Image;
	else if (typeName == "image_cpu")
		type = kVfxPlugType_ImageCpu;
	else if (typeName == "draw")
		type = kVfxPlugType_Draw;
	else if (typeName == "trigger")
		type = kVfxPlugType_Trigger;
	
	return type;
}

static void testVfxNodeCreation()
{
	VfxGraph vfxGraph;
	
	Assert(g_currentVfxGraph == nullptr);
	g_currentVfxGraph = &vfxGraph;
	
	for (VfxNodeTypeRegistration * registration = g_vfxNodeTypeRegistrationList; registration != nullptr; registration = registration->next)
	{
		const int64_t t1 = g_TimerRT.TimeUS_get();
		
		VfxNodeBase * vfxNode = registration->create();
		
		GraphNode node;
		
		vfxNode->initSelf(node);
		
		vfxNode->init(node);
		
		// check if vfx node is created properly
		
		Assert(vfxNode->inputs.size() == registration->inputs.size());
		const int numInputs = std::max(vfxNode->inputs.size(), registration->inputs.size());
		for (int i = 0; i < numInputs; ++i)
		{
			if (i >= vfxNode->inputs.size())
			{
				logError("input in registration doesn't exist in vfx node: index=%d, name=%s", i, registration->inputs[i].name.c_str());
			}
			else if (i >= registration->inputs.size())
			{
				logError("input in vfx node doesn't exist in registration: index=%d, type=%d", i, vfxNode->inputs[i].type);
			}
			else
			{
				auto & r = registration->inputs[i];
				
				const VfxPlugType type = stringToVfxPlugType(r.typeName);
				
				if (type == kVfxPlugType_None)
					logError("unknown type name in registration: index=%d, typeName=%s", i, r.typeName.c_str());
				else if (type != vfxNode->inputs[i].type)
					logError("different types in registration vs vfx node. index=%d, typeName=%s", i, r.typeName.c_str());
			}
		}
		
		Assert(vfxNode->outputs.size() == registration->outputs.size());
		const int numOutputs = std::max(vfxNode->outputs.size(), registration->outputs.size());
		for (int i = 0; i < numOutputs; ++i)
		{
			if (i >= vfxNode->outputs.size())
			{
				logError("output in registration doesn't exist in vfx node: index=%d, name=%s", i, registration->outputs[i].name.c_str());
			}
			else if (i >= registration->outputs.size())
			{
				logError("output in vfx node doesn't exist in registration: index=%d, type=%d", i, vfxNode->outputs[i].type);
			}
			else
			{
				auto & r = registration->outputs[i];
				
				const VfxPlugType type = stringToVfxPlugType(r.typeName);
				
				if (type == kVfxPlugType_None)
					logError("unknown type name in registration: index=%d, typeName=%s", i, r.typeName.c_str());
				else if (type != vfxNode->outputs[i].type)
					logError("different types in registration vs vfx node. index=%d, typeName=%s", i, r.typeName.c_str());
			}
		}
		
		delete vfxNode;
		vfxNode = nullptr;
		
		const int64_t t2 = g_TimerRT.TimeUS_get();
		
		logDebug("node create/destroy took %dus. nodeType=%s", t2 - t1, registration->typeName.c_str()); (void)t1; (void)t2;
	}
	
	g_currentVfxGraph = nullptr;
}

//

static void testDynamicInputs()
{
	VfxGraph g;
	
	VfxNodeBase * node1 = new VfxNodeBase();
	VfxNodeBase * node2 = new VfxNodeBase();
	
	g.nodes[0] = node1;
	g.nodes[1] = node2;
	
	float values[32];
	for (int i = 0; i < 32; ++i)
		values[i] = i;
	
	node1->resizeSockets(1, 2);
	node2->resizeSockets(2, 1);
	
	node1->addInput(0, kVfxPlugType_Float);
	node1->addOutput(0, kVfxPlugType_Float, &values[0]);
	node1->addOutput(1, kVfxPlugType_Float, &values[1]);
	node2->addInput(0, kVfxPlugType_Float);
	node2->addInput(1, kVfxPlugType_Float);
	node2->addOutput(0, kVfxPlugType_Float, &values[16]);
	
	node1->tryGetInput(0)->connectTo(*node2->tryGetOutput(0));
	node2->tryGetInput(0)->connectTo(*node1->tryGetOutput(0));
	node2->tryGetInput(1)->connectTo(*node1->tryGetOutput(1));
	
	{
		VfxDynamicLink link;
		link.srcNodeId = 0;
		link.srcSocketName = "a";
		link.srcSocketIndex = -1;
		link.dstNodeId = 1;
		link.dstSocketIndex = 0;
		g.dynamicData->links.push_back(link);
	}
	
	{
		VfxDynamicLink link;
		link.srcNodeId = 0;
		link.srcSocketName = "b";
		link.srcSocketIndex = -1;
		link.dstNodeId = 1;
		link.dstSocketIndex = 0;
		g.dynamicData->links.push_back(link);
	}
	
	{
		VfxDynamicLink link;
		link.srcNodeId = 0;
		link.srcSocketName = "b";
		link.srcSocketIndex = -1;
		link.dstNodeId = 1;
		link.dstSocketName = "a";
		link.dstSocketIndex = -1;
		g.dynamicData->links.push_back(link);
	}
	
	g_currentVfxGraph = &g;
	{
		VfxNodeBase::DynamicInput inputs[2];
		inputs[0].name = "a";
		inputs[0].type = kVfxPlugType_Float;
		inputs[1].name = "b";
		inputs[1].type = kVfxPlugType_Float;
		
		node1->setDynamicInputs(inputs, 2);
		
		node1->setDynamicInputs(nullptr, 0);
		
		node1->setDynamicInputs(inputs, 2);
		
		//
		
		float value = 1.f;
		
		VfxNodeBase::DynamicOutput outputs[1];
		outputs[0].name = "a";
		outputs[0].type = kVfxPlugType_Float;
		outputs[0].mem = &value;
		
		node2->setDynamicOutputs(outputs, 1);
		
		node2->setDynamicOutputs(nullptr, 0);
		
		node2->setDynamicOutputs(outputs, 1);
	}
	g_currentVfxGraph = nullptr;
}

//

#include "Path.h"

static std::string filedrop;

static void handleAction(const std::string & action, const Dictionary & args)
{
	if (action == "filedrop")
	{
		const Path basepath(getDirectory());
		const Path filepath(args.getString("file", ""));
		
		Path relativePath;
		relativePath.MakeRelative(basepath, filepath);
		
		filedrop = relativePath.ToString();
	}
}

static void handleRealTimeEdit(const std::string & filename)
{
	if (String::StartsWith(filename, "fsfx/"))
	{
		// todo : reload shader
	}
}

//

#include "tinyxml2_helpers.h"

static void checkVfxGraphIntegrity(const char * filename, const GraphEdit_TypeDefinitionLibrary & tdl)
{
	tinyxml2::XMLDocument d;
	
	if (d.LoadFile(filename) != XML_SUCCESS)
	{
		logError("%s: failed to load XML document", filename);
	}
	else
	{
		auto xmlGraph = d.FirstChildElement("graph");
		
		if (xmlGraph == nullptr)
		{
			logError("%s: failed to find graph element", filename);
		}
		else
		{
			std::map<GraphNodeId, std::string> nodes;
			
			for (auto xmlNode = xmlGraph->FirstChildElement("node"); xmlNode != nullptr; xmlNode = xmlNode->NextSiblingElement("node"))
			{
				const GraphNodeId nodeId = intAttrib(xmlNode, "id", kGraphNodeIdInvalid);
				const char * typeName = stringAttrib(xmlNode, "typeName", nullptr);
				
				if (nodeId == kGraphNodeIdInvalid)
				{
					logError("%s: found node without valid node id", filename);
				}
				else
				{
					auto i = nodes.find(nodeId);
					
					if (i != nodes.end())
					{
						logError("%s: found node with duplicate node id. nodeId=%d", filename, nodeId);
					}
					else
					{
						nodes.insert(std::make_pair(nodeId, typeName ? typeName : ""));
					}
				}
				
				if (typeName == nullptr)
				{
					logError("%s: found node without valid type name. nodeId=%d", filename, nodeId);
				}
				else
				{
					auto td = tdl.tryGetTypeDefinition(typeName);
					
					if (td == nullptr)
					{
						logError("%s: found node with type name that doesn't exist in type definition library. nodeId=%d, typeName=%s", filename, nodeId, typeName);
					}
				}
			}
			
			//
			
			std::set<GraphLinkId> linkIds;
			
			for (auto xmlLink = xmlGraph->FirstChildElement("link"); xmlLink != nullptr; xmlLink = xmlLink->NextSiblingElement("link"))
			{
				const int linkId = intAttrib(xmlLink, "id", kGraphLinkIdInvalid);
				const bool isDynamic = boolAttrib(xmlLink, "dynamic", false);
				
				if (linkId == kGraphLinkIdInvalid)
				{
					logError("%s: found link without valid link id", filename);
				}
				else
				{
					auto i = linkIds.find(linkId);
					
					if (i != linkIds.end())
					{
						logError("%s: found link with duplicate link id. linkId=%d", filename, linkId);
					}
					else
					{
						linkIds.insert(linkId);
					}
				}
				
				const int srcNodeId = intAttrib(xmlLink, "srcNodeId", kGraphNodeIdInvalid);
				const char * srcNodeSocketName = stringAttrib(xmlLink, "srcNodeSocketName", nullptr);
				const int dstNodeId = intAttrib(xmlLink, "dstNodeId", kGraphNodeIdInvalid);
				const char * dstNodeSocketName = stringAttrib(xmlLink, "dstNodeSocketName", nullptr);
				
				if (srcNodeId == kGraphNodeIdInvalid)
					logError("%s: found link with invalid src node id", filename);
				if (srcNodeSocketName == nullptr)
					logError("%s: found link with invalid src socket name", filename);
				if (dstNodeId == kGraphNodeIdInvalid)
					logError("%s: found link with invalid dst node id", filename);
				if (dstNodeSocketName == nullptr)
					logError("%s: found link with invalid dst socket name", filename);
				
				if (srcNodeId != kGraphNodeIdInvalid)
				{
					auto i = nodes.find(srcNodeId);
					
					if (i == nodes.end())
						logError("%s: found link with non-existing src node. linkId=%d, srcNodeId=%d", filename, linkId, srcNodeId);
					else
					{
						auto td = tdl.tryGetTypeDefinition(i->second);
						
						if (td != nullptr)
						{
							if (srcNodeSocketName != nullptr)
							{
								bool found = false;
								
								for (auto & input : td->inputSockets)
									if (input.name == srcNodeSocketName)
										found = true;
								
								if (found == false && isDynamic == false)
								{
									logError("%s: found link with non-existing src socket. linkId=%d, srcNodeId=%d, srcNodeSocketName=%s", filename, linkId, srcNodeId, srcNodeSocketName);
								}
							}
						}
					}
				}
				
				if (dstNodeId != kGraphNodeIdInvalid)
				{
					auto i = nodes.find(dstNodeId);
					
					if (i == nodes.end())
						logError("%s: found link with non-existing dst node. linkId=%d, dstNodeId=%d", filename, linkId, dstNodeId);
					else
					{
						auto td = tdl.tryGetTypeDefinition(i->second);
						
						if (td != nullptr)
						{
							if (dstNodeSocketName != nullptr)
							{
								bool found = false;
								
								for (auto & output : td->outputSockets)
									if (output.name == dstNodeSocketName)
										found = true;
								
								if (found == false && isDynamic == false)
								{
									logError("%s: found link with non-existing dst socket. linkId=%d, dstNodeId=%d, dstNodeSocketName=%s", filename, linkId, dstNodeId, dstNodeSocketName);
								}
							}
						}
					}
				}
			}
		}
	}
}

static void checkVfxGraphs(const GraphEdit_TypeDefinitionLibrary & tdl)
{
	std::vector<std::string> filenames = listFiles(".", true);
	
	for (auto & filename : filenames)
	{
		if (Path::GetExtension(filename, true) != "xml")
			continue;
		
		if (filename == "audioKey.xml" ||
			filename == "audioKey2.xml" ||
			filename == "ccl.xml" ||
			filename == "combTest5.xml" ||
			filename == "combTest5p.xml" ||
			filename == "mlworkshopA.xml" ||
			filename == "mlworkshopAp.xml" ||
			filename == "types.xml")
			continue;
		
		checkVfxGraphIntegrity(filename.c_str(), tdl);
	}
}

//

int main(int argc, char * argv[])
{
	framework.enableRealTimeEditing = true;
	
	framework.enableDepthBuffer = false;
	//framework.enableDrawTiming = true;
	//framework.enableProfiling = true;
	
	framework.filedrop = true;
	framework.actionHandler = handleAction;
	framework.realTimeEditCallback = handleRealTimeEdit;
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		initUi();
		
		MP::Util::InitializeLibAvcodec();
		
		vfxSetThreadName("Main Thread");
		
		testVfxNodeCreation();
		
		//testDynamicInputs();
		
		//
		
	#ifndef DEBUG
		framework.fillCachesWithPath(".", true);
	#endif
		
		//
		
		GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary = new GraphEdit_TypeDefinitionLibrary();
		
		createVfxTypeDefinitionLibrary(*typeDefinitionLibrary, g_vfxEnumTypeRegistrationList, g_vfxNodeTypeRegistrationList);
		
		//
		
		if (true)
		{
			checkVfxGraphs(*typeDefinitionLibrary);
		}
		
		//
		
		if (false)
		{
			// stress test vfx graph creation and destruction. enable this code path when profiling only
			
			Graph graph;
			
			XMLDocument document;
			
			if (document.LoadFile(FILENAME) == XML_SUCCESS)
			{
				const XMLElement * xmlGraph = document.FirstChildElement("graph");
				
				if (xmlGraph != nullptr)
				{
					graph.loadXml(xmlGraph, typeDefinitionLibrary);
					
					int i = 0;
					
					for (;;)
					{
						auto t1 = g_TimerRT.TimeUS_get();
						
						auto vfxGraph = constructVfxGraph(graph, typeDefinitionLibrary);
						
						vfxGraph->tick(GFX_SX, GFX_SY, 0.f);
						
						delete vfxGraph;
						vfxGraph = nullptr;
						
						auto t2 = g_TimerRT.TimeUS_get();
						printf("construct %003d + tick took %dus\n", i, int(t2 - t1));
						
						++i;
					}
				}
			}
		}
		
		//
		
		VfxGraph * vfxGraph = new VfxGraph();
		
		RealTimeConnection * realTimeConnection = new RealTimeConnection(vfxGraph);
		
		GraphEdit * graphEdit = new GraphEdit(GFX_SX, GFX_SY, typeDefinitionLibrary, realTimeConnection);
		
		//
		
		graphEdit->load(FILENAME);
		
		//
		
		Surface * graphEditSurface = new Surface(GFX_SX, GFX_SY, false);
		
		//
		
		float realtimePreviewAnim = 1.f;
		
		double vflip = 1.0;
		
		SDL_StartTextInput();
		
		while (!framework.quitRequested)
		{
			vfxCpuTimingBlock(tick);
			vfxGpuTimingBlock(tick);
			
			// todo : remove. test here to estimate how much memory using serialization as the undo/redo mechanism would use
			if (false && keyboard.wentDown(SDLK_m))
			{
				std::vector<std::string> historyList;
				historyList.resize(1000);
				
				for (auto & history : historyList)
				{
					bool result = true;
					
					tinyxml2::XMLPrinter xmlGraph;
					
					xmlGraph.OpenElement("graph");
					{
						result &= graphEdit->graph->saveXml(xmlGraph, graphEdit->typeDefinitionLibrary);
						
						xmlGraph.OpenElement("editor");
						{
							result &= graphEdit->saveXml(xmlGraph);
						}
						xmlGraph.CloseElement();
					}
					xmlGraph.CloseElement();
					
					Assert(result);
					if (result)
					{
						history = xmlGraph.CStr();
					}
				}
				
				for (;;);
			}
			
			// when real-time preview is disabled and all animations are done, wait for events to arrive. otherwise just keep processing and drawing frames
			
			if (graphEdit->editorOptions.realTimePreview == false && realtimePreviewAnim == 0.f && graphEdit->animationIsDone)
				framework.waitForEvents = true;
			else
				framework.waitForEvents = false;
			
			framework.process();
			
			// the time step may be large when waiting for events. to avoid animation hitches we set it to zero here. when the processing below activates an animation we'll use the actual time step we get
			
			if (framework.waitForEvents)
				framework.timeStep = 0.f;
			
			//
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			// avoid excessively large time steps by clamping it to 1/15th of a second, simulating a minimum of 15 fps
			
			framework.timeStep = std::min(framework.timeStep, 1.f / 15.f);
			
			const float dt = framework.timeStep;
			
			//
			
			g_oscEndpointMgr.tick();
			
			//
			
			bool inputIsCaptured = false;
			
			//
			
			if (filedrop.empty() == false)
			{
				graphEdit->load(filedrop.c_str());
				
				graphEdit->idleTime = 0.f;
				
				filedrop.clear();
			}
			
			//
			
			inputIsCaptured |= graphEdit->tick(dt, inputIsCaptured);
			
			if (inputIsCaptured == false)
			{
				// todo : move this to graph edit ?
				// todo : dynamically get src/dst socket indices from type definition
				// todo : pop up a node type select dialog ?
				
				if (mouse.wentDown(BUTTON_LEFT) && keyboard.isDown(SDLK_RSHIFT))
				{
					GraphEdit::HitTestResult hitTestResult;
					
					if (graphEdit->hitTest(graphEdit->mousePosition.x, graphEdit->mousePosition.y, hitTestResult))
					{
						if (hitTestResult.hasLink)
						{
							auto linkTypeDefinition = graphEdit->tryGetLinkTypeDefinition(hitTestResult.link->id);
							
							if (linkTypeDefinition != nullptr)
							{
								if (linkTypeDefinition->srcTypeName == "draw" && linkTypeDefinition->dstTypeName == "draw")
								{
									GraphLink link = *hitTestResult.link;
									
									GraphEdit::LinkPath linkPath;
									
									if (graphEdit->getLinkPath(link.id, linkPath))
									{
										GraphNodeId nodeId;
										
										if (graphEdit->tryAddNode(
											"draw.fsfx-v2",
											graphEdit->mousePosition.x,
											graphEdit->mousePosition.y,
											true, &nodeId))
										{
											if (true)
											{
												GraphLink link1;
												link1.id = graphEdit->graph->allocLinkId();
												link1.srcNodeId = link.srcNodeId;
												link1.srcNodeSocketIndex = link.srcNodeSocketIndex;
												link1.srcNodeSocketName = link.srcNodeSocketName;
												link1.dstNodeId = nodeId;
												link1.dstNodeSocketIndex = 0;
												link1.dstNodeSocketName = "any";
												graphEdit->graph->addLink(link1, true);
											}
											
											if (true)
											{
												GraphLink link2;
												link2.id = graphEdit->graph->allocLinkId();
												link2.srcNodeId = nodeId;
												link2.srcNodeSocketIndex = 0;
												link2.srcNodeSocketName = "before";
												link2.dstNodeId = link.dstNodeId;
												link2.dstNodeSocketIndex = link.dstNodeSocketIndex;
												link2.dstNodeSocketName = link.dstNodeSocketName;
												graphEdit->graph->addLink(link2, true);
											}
										}
									}
								}
							}
						}
					}
					
					inputIsCaptured = true;
				}
			}
			
			if (inputIsCaptured == false)
			{
				if (keyboard.wentDown(SDLK_p) && keyboard.isDown(SDLK_LGUI))
				{
					inputIsCaptured = true;
					
					graphEdit->editorOptions.realTimePreview = !graphEdit->editorOptions.realTimePreview;
				}
			}
			
			if (graphEdit->state == GraphEdit::kState_Hidden)
				SDL_ShowCursor(0);
			else
				SDL_ShowCursor(1);
			
			//
			
			if (graphEdit->editorOptions.realTimePreview)
			{
				realtimePreviewAnim = std::min(1.f, realtimePreviewAnim + dt / .3f);
			}
			else
			{
				realtimePreviewAnim = std::max(0.f, realtimePreviewAnim - dt / .5f);
			}
			
			if (vfxGraph != nullptr)
			{
				const double timeStep = dt * realtimePreviewAnim;
				
				vfxGraph->tick(GFX_SX, GFX_SY, timeStep);
			}
			
			// update vflip effect
			
			const double targetVflip = graphEdit->dragAndZoom.zoom < 0.f ? -1.0 : +1.0;
			const double vflipMix = std::pow(.001, dt);
			vflip = vflip * vflipMix + targetVflip * (1.0 - vflipMix);
			
			framework.beginDraw(0, 0, 0, 255);
			{
				vfxGpuTimingBlock(draw);
				
				if (vfxGraph != nullptr)
				{
					gxPushMatrix();
					{
						gxTranslatef(+GFX_SX/2, +GFX_SY/2, 0);
						gxScalef(1.f, vflip, 1.f);
						gxTranslatef(-GFX_SX/2, -GFX_SY/2, 0);
						
						vfxGraph->draw(GFX_SX, GFX_SY);
					}
					gxPopMatrix();
				}
				
				graphEdit->tickVisualizers(dt);
				
				pushSurface(graphEditSurface);
				{
					graphEditSurface->clear(0, 0, 0, 255);
					pushBlend(BLEND_ADD);
					
					glBlendEquation(GL_FUNC_ADD);
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
					
					graphEdit->draw();
					
					popBlend();
				}
				popSurface();
				
				const float hideTime = graphEdit->hideTime;
				
				if (hideTime > 0.f)
				{
					if (hideTime < 1.f)
					{
						pushBlend(BLEND_OPAQUE);
						{
							const float radius = std::pow(1.f - hideTime, 2.f) * 200.f;
							
							setShader_GaussianBlurH(graphEditSurface->getTexture(), 32, radius);
							graphEditSurface->postprocess();
							clearShader();
						}
						popBlend();
					}
					
					Shader composite("composite");
					setShader(composite);
					pushBlend(BLEND_ADD);
					{
						glBlendEquation(GL_FUNC_ADD);
						glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
					
						composite.setTexture("source", 0, graphEditSurface->getTexture(), false, true);
						composite.setImmediate("opacity", hideTime);
						
						drawRect(0, 0, GFX_SX, GFX_SY);
					}
					popBlend();
					clearShader();
				}
			}
			framework.endDraw();
		}
		
		SDL_StopTextInput();
		
		Font("calibri.ttf").saveCache();
		
		delete graphEditSurface;
		graphEditSurface = nullptr;
	
		delete graphEdit;
		graphEdit = nullptr;
		
		delete realTimeConnection;
		realTimeConnection = nullptr;
		
		delete typeDefinitionLibrary;
		typeDefinitionLibrary = nullptr;
		
		shutUi();
		
		framework.shutdown();
	}

	return 0;
}
