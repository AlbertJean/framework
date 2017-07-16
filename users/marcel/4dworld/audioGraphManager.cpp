#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioGraphRealTimeConnection.h"
#include "audioNodeBase.h"
#include "graph.h"

//

struct AudioGraphFileRTC : GraphEdit_RealTimeConnection
{
	AudioGraphFile * file;
	
	AudioGraphFileRTC()
		: GraphEdit_RealTimeConnection()
		, file(nullptr)
	{
	}
	
	virtual void loadBegin() override
	{
		for (auto & instance : file->instanceList)
			instance.realTimeConnection->loadBegin();
	}

	virtual void loadEnd(GraphEdit & graphEdit) override
	{
		for (auto & instance : file->instanceList)
			instance.realTimeConnection->loadEnd(graphEdit);
	}
	
	virtual void nodeAdd(const GraphNodeId nodeId, const std::string & typeName) override
	{
		for (auto & instance : file->instanceList)
			instance.realTimeConnection->nodeAdd(nodeId, typeName);
	}

	virtual void nodeRemove(const GraphNodeId nodeId) override
	{
		for (auto & instance : file->instanceList)
			instance.realTimeConnection->nodeRemove(nodeId);
	}

	virtual void linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		for (auto & instance : file->instanceList)
			instance.realTimeConnection->linkAdd(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
	}

	virtual void linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		for (auto & instance : file->instanceList)
			instance.realTimeConnection->linkRemove(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
	}
	
	virtual void setNodeIsPassthrough(const GraphNodeId nodeId, const bool isPassthrough) override
	{
		for (auto & instance : file->instanceList)
			instance.realTimeConnection->setNodeIsPassthrough(nodeId, isPassthrough);
	}
	
	virtual void setSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, const std::string & value) override
	{
		for (auto & instance : file->instanceList)
			instance.realTimeConnection->setSrcSocketValue(nodeId, srcSocketIndex, srcSocketName, value);
	}

	virtual bool getSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, std::string & value) override
	{
		return file->activeInstance->realTimeConnection->getSrcSocketValue(nodeId, srcSocketIndex, srcSocketName, value);
	}

	virtual void setDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, const std::string & value) override
	{
		for (auto & instance : file->instanceList)
			instance.realTimeConnection->setDstSocketValue(nodeId, dstSocketIndex, dstSocketName, value);
	}

	virtual bool getDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, std::string & value) override
	{
		return file->activeInstance->realTimeConnection->getDstSocketValue(nodeId, dstSocketIndex, dstSocketName, value);
	}
	
	virtual void clearSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName) override
	{
		for (auto & instance : file->instanceList)
			instance.realTimeConnection->clearSrcSocketValue(nodeId, srcSocketIndex, srcSocketName);
	}
	
	virtual bool getSrcSocketChannelData(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, GraphEdit_ChannelData & channels) override
	{
		return file->activeInstance->realTimeConnection->getSrcSocketChannelData(nodeId, srcSocketIndex, srcSocketName, channels);
	}

	virtual bool getDstSocketChannelData(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, GraphEdit_ChannelData & channels) override
	{
		return file->activeInstance->realTimeConnection->getDstSocketChannelData(nodeId, dstSocketIndex, dstSocketName, channels);
	}
	
	virtual void handleSrcSocketPressed(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName) override
	{
		file->activeInstance->realTimeConnection->handleSrcSocketPressed(nodeId, srcSocketIndex, srcSocketName);
	}
	
	virtual bool getNodeDescription(const GraphNodeId nodeId, std::vector<std::string> & lines) override
	{
		return file->activeInstance->realTimeConnection->getNodeDescription(nodeId, lines);
	}
	
	virtual int nodeIsActive(const GraphNodeId nodeId) override
	{
		return file->activeInstance->realTimeConnection->nodeIsActive(nodeId);
	}

	virtual int linkIsActive(const GraphLinkId linkId) override
	{
		return file->activeInstance->realTimeConnection->linkIsActive(linkId);
	}

	virtual int getNodeCpuHeatMax() const override
	{
		return file->activeInstance->realTimeConnection->getNodeCpuHeatMax();
	}

	virtual int getNodeCpuTimeUs(const GraphNodeId nodeId) const override
	{
		return file->activeInstance->realTimeConnection->getNodeCpuTimeUs(nodeId);
	}
};

//

AudioGraphInstance::AudioGraphInstance()
	: audioGraph(nullptr)
	, realTimeConnection(nullptr)
{
}

AudioGraphInstance::~AudioGraphInstance()
{
	delete audioGraph;
	audioGraph = nullptr;
	
	if (realTimeConnection != nullptr)
	{
		realTimeConnection->audioGraph = nullptr;
		realTimeConnection->audioGraphPtr = nullptr;
	}
	
	delete realTimeConnection;
	realTimeConnection = nullptr;
}

//

AudioGraphFile::AudioGraphFile()
	: filename()
	, instanceList()
	, activeInstance(nullptr)
	, realTimeConnection(nullptr)
	, graphEdit(nullptr)
{
	realTimeConnection = new AudioGraphFileRTC();
	realTimeConnection->file = this;
}

AudioGraphFile::~AudioGraphFile()
{
	instanceList.clear();
	
	activeInstance = nullptr;
	
	delete realTimeConnection;
	realTimeConnection = nullptr;
	
	delete graphEdit;
	graphEdit = nullptr;
}

//

AudioGraphManager::AudioGraphManager()
	: typeDefinitionLibrary(nullptr)
	, files()
{
	typeDefinitionLibrary = new GraphEdit_TypeDefinitionLibrary();
	
	{
		GraphEdit_ValueTypeDefinition typeDefinition;
		typeDefinition.typeName = "bool";
		typeDefinition.editor = "checkbox";
		typeDefinitionLibrary->valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	{
		GraphEdit_ValueTypeDefinition typeDefinition;
		typeDefinition.typeName = "int";
		typeDefinition.editor = "textbox_int";
		typeDefinition.visualizer = "valueplotter";
		typeDefinitionLibrary->valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	{
		GraphEdit_ValueTypeDefinition typeDefinition;
		typeDefinition.typeName = "float";
		typeDefinition.editor = "textbox_float";
		typeDefinition.visualizer = "valueplotter";
		typeDefinitionLibrary->valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	{
		GraphEdit_ValueTypeDefinition typeDefinition;
		typeDefinition.typeName = "string";
		typeDefinition.editor = "textbox";
		typeDefinitionLibrary->valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	{
		GraphEdit_ValueTypeDefinition typeDefinition;
		typeDefinition.typeName = "audioValue";
		typeDefinition.editor = "textbox_float";
		typeDefinition.visualizer = "channels";
		typeDefinitionLibrary->valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
	
	createAudioEnumTypeDefinitions(*typeDefinitionLibrary, g_audioEnumTypeRegistrationList);
	createAudioNodeTypeDefinitions(*typeDefinitionLibrary, g_audioNodeTypeRegistrationList);
}

AudioGraphManager::~AudioGraphManager()
{
	for (auto & file : files)
		delete file.second;
	files.clear();
	
	delete typeDefinitionLibrary;
	typeDefinitionLibrary = nullptr;
}

AudioGraphInstance * AudioGraphManager::createInstance(const char * filename)
{
	AudioGraphFile *& file = files[filename];
	
	if (file == nullptr)
	{
		file = new AudioGraphFile();
	}
	
	if (file->graphEdit == nullptr)
	{
		file->filename = filename;
		
		file->graphEdit = new GraphEdit(typeDefinitionLibrary);
		file->graphEdit->realTimeConnection = file->realTimeConnection;
		
		file->graphEdit->load(filename);
	}
	
	file->instanceList.push_back(AudioGraphInstance());
	AudioGraphInstance & instance = file->instanceList.back();
	
	instance.audioGraph = constructAudioGraph(*file->graphEdit->graph, typeDefinitionLibrary);
	instance.realTimeConnection = new AudioRealTimeConnection();
	instance.realTimeConnection->audioGraph = instance.audioGraph;
	instance.realTimeConnection->audioGraphPtr = &instance.audioGraph;
	
	if (file->activeInstance == nullptr)
	{
		file->activeInstance = &instance;
	}
	
	return &instance;
}

void AudioGraphManager::free(AudioGraphInstance *& instance)
{
	for (auto fileItr = files.begin(); fileItr != files.end(); ++fileItr)
	{
		auto & file = fileItr->second;
		
		for (auto instanceItr = file->instanceList.begin(); instanceItr != file->instanceList.end(); ++instanceItr)
		{
			if (&(*instanceItr) == instance)
			{
				file->instanceList.erase(instanceItr);
				instance = nullptr;
				
				if (file->instanceList.empty())
				{
					delete file;
					file = nullptr;
					
					files.erase(fileItr);
				}
				
				return;
			}
		}
	}
}

void AudioGraphManager::tick(const float dt)
{
	for (auto & file : files)
		for (auto & instance : file.second->instanceList)
			instance.audioGraph->tick(dt, true);
}

void AudioGraphManager::draw() const
{
	//for (auto & file : files)
	//	for (auto & instance : file.second.instanceList)
	//		instance.audioGraph->draw(..);
}

void AudioGraphManager::tickEditor(const float dt)
{
	for (auto & file : files)
		file.second->graphEdit->tick(dt);
}

void AudioGraphManager::drawEditor()
{
	for (auto & file : files)
		file.second->graphEdit->draw();
}
