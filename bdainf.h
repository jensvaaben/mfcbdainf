// MFCBDAINF - BDA DTV topology information application
// Copyright (C) 2014 Jens Vaaben <info@jensvaaben.com>, https://www.google.com/+JensVaaben http://www.dvbstreamexplorer.com

#if !defined(_BDAINF_H_)
#define _BDAINF_H_

#include <ks.h>
#include <bdatypes.h>
#include <ksmedia.h>
#include <bdamedia.h>
#include <string>
#include <vector>
#include <map>

typedef struct tagPININFO
{
	//KSPROPERTY_PIN_CATEGORY
	GUID category;
	bool category_valid;

	//KSPROPERTY_PIN_CINSTANCES
	KSPIN_CINSTANCES cinstances;
	bool cinstances_valid;

	//KSPROPERTY_PIN_COMMUNICATION
	KSPIN_COMMUNICATION communication;
	bool communication_valid;

	//KSPROPERTY_PIN_CONSTRAINEDDATARANGES
	bool constraineddataranges_valid;

	//KSPROPERTY_PIN_DATAFLOW
	KSPIN_DATAFLOW dataflow;
	bool dataflow_valid;

	//KSPROPERTY_PIN_DATAINTERSECTION
	KSDATAFORMAT dataintersection;
	bool dataintersection_valid;

	//KSPROPERTY_PIN_DATARANGES
	std::vector<KSDATARANGE> dataranges;

	//KSPROPERTY_PIN_GLOBALCINSTANCES
	KSPIN_CINSTANCES globalcinstances;
	bool globalcinstances_valid;

	//KSPROPERTY_PIN_INTERFACES
	std::vector<KSPIN_INTERFACE> interfaces;

	//KSPROPERTY_PIN_MEDIUMS
	std::vector<KSPIN_MEDIUM> mediums;

	//KSPROPERTY_PIN_NAME
	std::wstring name;

	//KSPROPERTY_PIN_NECESSARYINSTANCES
	ULONG necessaryinstances;
	bool necessaryinstances_valid;

	//KSPROPERTY_PIN_PHYSICALCONNECTION
	KSPIN_PHYSICALCONNECTION physicalconnection;
	std::wstring symboliclinkname;
	bool physicalconnection_valid;

	//KSPROPERTY_PIN_PROPOSEDATAFORMAT
	//KSDATAFORMAT proposedataformat;
	//bool proposedataformat_valid;

	void dump_device(FILE* f,int indent) const;

	tagPININFO() : category_valid(false),cinstances_valid(false),communication_valid(false),constraineddataranges_valid(false),
		dataflow_valid(false),dataintersection_valid(false),globalcinstances_valid(false),
		necessaryinstances_valid(false),physicalconnection_valid(false)/*,proposedataformat_valid(false)*/
	{
	}

} PININFO;

typedef struct tagPINTOPOLOGY
{
	ULONG pin_ctypes;
	std::vector<PININFO> pininfo;

	void dump_device(FILE* f,int indent) const;

	tagPINTOPOLOGY() : pin_ctypes(0)
	{
	}

} PINTOPOLOGY;

typedef struct tagTOPOLOGY
{
	// KSPROPERTY_TOPOLOGY_CATEGORIES
	std::vector<GUID> categories;

	//KSTOPOLOGY_CONNECTION
	std::vector<KSTOPOLOGY_CONNECTION> connection;

	//KSPROPERTY_TOPOLOGY_NAME
	std::vector<std::wstring> name;

	//KSPROPERTY_TOPOLOGY_NODES
	std::vector<GUID> nodes;

	void dump_device(FILE* f,int indent) const;
} TOPOLOGY;

typedef struct tagBDATOPOLOGY // KSPROPSETID_BdaTopology
{
	std::vector<BDA_TEMPLATE_CONNECTION> bda_template_connection; // KSPROPERTY_BDA_TEMPLATE_CONNECTIONS
	std::vector<BDANODE_DESCRIPTOR> bdanode_descriptor; // KSPROPERTY_BDA_NODE_DESCRIPTORS
	std::map<unsigned long,std::vector<GUID> > bda_node_properties; // KSPROPERTY_BDA_NODE_PROPERTIES
	std::vector<unsigned long> bda_node_types; // KSPROPERTY_BDA_NODE_TYPES
	// KSPROPERTY_BDA_CONTROLLING_PIN_ID
	std::vector<unsigned long> bda_pin_types; // KSPROPERTY_BDA_PIN_TYPES

	std::map<unsigned long,std::vector<GUID> > bda_node_methods; // KSPROPERTY_BDA_NODE_METHODS
	std::map<unsigned long,std::vector<GUID> > bda_node_events; // KSPROPERTY_BDA_NODE_EVENTS


	void dump_device(FILE* f,int indent) const;

} BDATOPOLOGY;

typedef struct tagDEVICE
{
	HANDLE hFile;

	std::wstring device_path;
	std::wstring DeviceDesc;

	std::wstring Service;
	std::wstring Class;
	std::wstring ClassGUID;
	std::wstring Driver;
	std::wstring TechnoTrend;
	std::wstring PhysicalDeviceObjectName;
	std::wstring Enumerator_Name;
	std::wstring device_instance_id;

	BDATOPOLOGY bdatopology;
	TOPOLOGY topology;
	PINTOPOLOGY pintopology;

	void dump_device(FILE* f,int indent) const;
	static void do_indent(FILE* f,int indent);

} DEVICE;

typedef std::vector<DEVICE> DEVICELIST;

typedef struct tagBDADEVICES
{
	DEVICELIST bda_source;
	DEVICELIST bda_reciever;

	void reset()
	{
		bda_source.clear();
		bda_reciever.clear();
	}

} BDADEVICES;

void scan_bda_decives(BDADEVICES&);
void guid_to_string(const GUID* guid,std::wstring& str);
void dump_device_list(FILE* f,const BDADEVICES& devices);

#endif