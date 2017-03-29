#pragma once

#include "Mat4x4.h"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace tinyxml2
{
	class XMLElement;
	class XMLPrinter;
}

typedef unsigned int GraphNodeId;
typedef unsigned int GraphLinkId;

extern GraphNodeId kGraphNodeIdInvalid;
extern GraphLinkId kGraphLinkIdInvalid;

struct GraphNode
{
	GraphNodeId id;
	std::string typeName;
	
	float editorX;
	float editorY;
	
	std::string editorValue;
	bool editorIsPassthrough;
	
	GraphNode();
	
	void togglePassthrough();
};

struct GraphNodeSocketLink
{
	GraphLinkId id;
	
	GraphNodeId srcNodeId;
	int srcNodeSocketIndex;
	
	GraphNodeId dstNodeId;
	int dstNodeSocketIndex;
	
	GraphNodeSocketLink();
};

struct Graph
{
	std::map<GraphNodeId, GraphNode> nodes;
	std::map<GraphLinkId, GraphNodeSocketLink> links;
	
	GraphNodeId nextNodeId;
	GraphLinkId nextLinkId;
	
	Graph();
	~Graph();
	
	GraphNodeId allocNodeId();
	GraphLinkId allocLinkId();
	
	void addNode(GraphNode & node);
	void removeNode(const GraphNodeId nodeId);
	void removeLink(const GraphLinkId linkId);
	
	bool loadXml(const tinyxml2::XMLElement * xmlGraph);
	bool saveXml(tinyxml2::XMLPrinter & xmlGraph) const;
};

//

struct GraphEdit_Editor
{
	std::string typeName;
	int outputSocketIndex;
	
	// editor
	
	float editorX;
	float editorY;
	float editorSx;
	float editorSy;
	
	GraphEdit_Editor()
		: typeName()
		, outputSocketIndex(-1)
		, editorX(0.f)
		, editorY(0.f)
		, editorSx(0.f)
		, editorSy(0.f)
	{
	}
};

//

struct GraphEdit_TypeDefinition
{
	struct InputSocket;
	struct OutputSocket;
	
	struct InputSocket
	{
		std::string typeName;
		
		// ui
		
		std::string displayName;
		
		int index;
		float px;
		float py;
		float radius;
		
		InputSocket()
			: typeName()
			, displayName()
			, index(-1)
			, px(0.f)
			, py(0.f)
			, radius(0.f)
		{
		}
		
		bool canConnectTo(const OutputSocket & socket) const;
	};
	
	struct OutputSocket
	{
		std::string typeName;
		
		// ui
		
		std::string displayName;
		
		int index;
		float px;
		float py;
		float radius;
		
		OutputSocket()
			: typeName()
			, displayName()
			, index(-1)
			, px(0.f)
			, py(0.f)
			, radius(0.f)
		{
		}
		
		bool canConnectTo(const InputSocket & socket) const;
	};
	
	struct HitTestResult
	{
		const GraphEdit_Editor * editor;
		const InputSocket * inputSocket;
		const OutputSocket * outputSocket;
		bool background;
		
		// todo : move to cpp
		HitTestResult()
			: editor(nullptr)
			, inputSocket(nullptr)
			, outputSocket(nullptr)
			, background(false)
		{
		}
	};
	
	std::string typeName;
	
	std::vector<GraphEdit_Editor> editors;
	std::vector<InputSocket> inputSockets;
	std::vector<OutputSocket> outputSockets;
	
	// ui
	
	std::string displayName;
	
	float sx;
	float sy;
	
	// todo : move to cpp
	GraphEdit_TypeDefinition()
		: typeName()
		, editors()
		, inputSockets()
		, outputSockets()
		, sx(0.f)
		, sy(0.f)
	{
	}
	
	void createUi();
	
	bool hitTest(const float x, const float y, HitTestResult & result) const;
	
	void loadXml(const tinyxml2::XMLElement * xmlLibrary);
};

struct GraphEdit_TypeDefinitionLibrary
{
	std::map<std::string, GraphEdit_TypeDefinition> typeDefinitions;
	
	// todo : move to cpp
	GraphEdit_TypeDefinitionLibrary()
		: typeDefinitions()
	{
	}
	
	// todo : move to cpp
	const GraphEdit_TypeDefinition * tryGetTypeDefinition(const std::string & typeName) const
	{
		auto i = typeDefinitions.find(typeName);
		
		if (i != typeDefinitions.end())
			return &i->second;
		else
			return nullptr;
	}
	
	void loadXml(const tinyxml2::XMLElement * xmlLibrary);
};

//

struct GraphEdit
{
	enum State
	{
		kState_Idle,
		kState_NodeSelect,
		kState_NodeDrag,
		kState_InputSocketConnect,
		kState_OutputSocketConnect,
		kState_SocketValueEdit,
		kState_Hidden
	};
	
	struct HitTestResult
	{
		bool hasNode;
		GraphNode * node;
		GraphEdit_TypeDefinition::HitTestResult nodeHitTestResult;
		
		bool hasLink;
		GraphNodeSocketLink * link;
		
		HitTestResult()
			: hasNode(false)
			, node(nullptr)
			, nodeHitTestResult()
			, hasLink(false)
			, link(nullptr)
		{
		}
	};
	
	struct SocketSelection
	{
		GraphNodeId srcNodeId;
		const GraphEdit_TypeDefinition::InputSocket * srcNodeSocket;
		GraphNodeId dstNodeId;
		const GraphEdit_TypeDefinition::OutputSocket * dstNodeSocket;
		
		SocketSelection()
			: srcNodeId(kGraphNodeIdInvalid)
			, srcNodeSocket(nullptr)
			, dstNodeId(kGraphNodeIdInvalid)
			, dstNodeSocket(nullptr)
		{
		}
	};
	
	struct DragAndZoom
	{
		float zoom;
		float focusX;
		float focusY;
		
		Mat4x4 transform;
		Mat4x4 invTransform;
		
		DragAndZoom()
			: zoom(1.f)
			, focusX(0.f)
			, focusY(0.f)
			, transform(true)
			, invTransform(true)
		{
			updateTransform();
		}
		
		void tick(const float dt)
		{
			updateTransform();
		}
		
		void updateTransform()
		{
			transform = Mat4x4(true).Translate(1024/2, 768/2, 0).Scale(zoom, zoom, 1.f).Translate(-focusX, -focusY, 0.f);
			invTransform = transform.Invert();
		}
	};
	
	struct GraphEditMouse
	{
		float x;
		float y;
		
		GraphEditMouse()
			: x(0.f)
			, y(0.f)
		{
		}
	};
	
	// state support structures
	
	struct NodeSelect
	{
		int beginX;
		int beginY;
		int endX;
		int endY;
		
		std::set<GraphNodeId> nodeIds;
		
		NodeSelect()
			: beginX(0)
			, beginY(0)
			, endX(0)
			, endY(0)
			, nodeIds()
		{
		}
	};
	
	struct SocketConnect
	{
		GraphNodeId srcNodeId;
		const GraphEdit_TypeDefinition::InputSocket * srcNodeSocket;
		GraphNodeId dstNodeId;
		const GraphEdit_TypeDefinition::OutputSocket * dstNodeSocket;
		
		SocketConnect()
			: srcNodeId(kGraphNodeIdInvalid)
			, srcNodeSocket(nullptr)
			, dstNodeId(kGraphNodeIdInvalid)
			, dstNodeSocket(nullptr)
		{
		}
	};
	
	struct SocketValueEdit
	{
		enum Mode
		{
			kMode_Idle,
			kMode_MouseAbsolute,
			kMode_MouseRelative,
			kMode_Keyboard
		};
		
		GraphNodeId nodeId;
		const GraphEdit_Editor * editor;
		Mode mode;
		std::string keyboardText;
		
		SocketValueEdit()
			: nodeId(kGraphNodeIdInvalid)
			, editor(nullptr)
			, mode(kMode_Idle)
		{
		}
		
		bool processKeyboard();
	};
	
	Graph * graph;
	
	GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary;
	
	std::set<GraphNodeId> selectedNodes;
	std::set<GraphLinkId> highlightedLinks;
	std::set<GraphLinkId> selectedLinks;
	
	SocketSelection highlightedSockets;
	SocketSelection selectedSockets;
	
	State state;
	
	NodeSelect nodeSelect;
	SocketConnect socketConnect;
	SocketValueEdit socketValueEdit;
	
	GraphEditMouse mousePosition;
	
	DragAndZoom dragAndZoom;
	
	GraphEdit();
	~GraphEdit();
	
	GraphNode * tryGetNode(const GraphNodeId id) const;
	const GraphEdit_TypeDefinition::InputSocket * tryGetInputSocket(const GraphNodeId nodeId, const int socketIndex) const;
	const GraphEdit_TypeDefinition::OutputSocket * tryGetOutputSocket(const GraphNodeId nodeId, const int socketIndex) const;

	bool hitTest(const float x, const float y, HitTestResult & result) const;
	
	void tick(const float dt);
	void nodeSelectEnd();
	void socketConnectEnd();
	void socketValueEditEnd();
	
	void selectNode(const GraphNodeId nodeId);
	void selectLink(const GraphLinkId linkId);
	void selectNodeAll();
	void selectLinkAll();
	void selectAll();
	
	void draw() const;
	void drawTypeUi(const GraphNode & node, const GraphEdit_TypeDefinition & typeDefinition) const;
};
