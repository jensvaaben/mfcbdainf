// MFCBDAINF - BDA DTV topology information application
// Copyright (C) 2014 Jens Vaaben <info@jensvaaben.com>, https://www.google.com/+JensVaaben http://www.dvbstreamexplorer.com

#include <stdafx.h>
#include <msxml2.h>
#include <comip.h>
#include "bdainf.h"

_COM_SMARTPTR_TYPEDEF(IXMLDOMDocument,__uuidof(IXMLDOMDocument));
_COM_SMARTPTR_TYPEDEF(IXMLDOMElement,__uuidof(IXMLDOMElement));
_COM_SMARTPTR_TYPEDEF(IXMLDOMNode ,__uuidof(IXMLDOMNode ));
_COM_SMARTPTR_TYPEDEF(IXMLDOMProcessingInstruction,__uuidof(IXMLDOMProcessingInstruction));
_COM_SMARTPTR_TYPEDEF(IXMLDOMAttribute,__uuidof(IXMLDOMAttribute));
_COM_SMARTPTR_TYPEDEF(IXMLDOMNodeList,__uuidof(IXMLDOMNodeList));
_COM_SMARTPTR_TYPEDEF(IXMLDOMNamedNodeMap,__uuidof(IXMLDOMNamedNodeMap));

static void CreatePI(IXMLDOMDocument* pDoc);
static void AppendChild(IXMLDOMNode* pChild, IXMLDOMNode* pParent);
static void CreateElement(IXMLDOMDocument* pDoc, LPCWSTR name,IXMLDOMElement** ppElement);
static void CreateDeviceList(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const DEVICELIST& l);
static void AddDevice(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const DEVICE& d);
static void AddAttribute(IXMLDOMDocument* pDoc,LPCWSTR name, LPCWSTR value,IXMLDOMElement* pParent);
static void AddAttribute(IXMLDOMDocument* pDoc,LPCWSTR name, _variant_t& v,IXMLDOMElement* pParent);
static void AddBDATopology(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const BDATOPOLOGY& t);
static void AddTopology(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const TOPOLOGY& t);
static void AddPinTopology(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const PINTOPOLOGY& t);
static void AddBDATemplateConnection(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent, const BDA_TEMPLATE_CONNECTION& t);
static void AddTopologyConnection(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent, const KSTOPOLOGY_CONNECTION& t);
static void AddBDANodeDescriptor(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent, const BDANODE_DESCRIPTOR& t);
static void AddGUIDList(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,std::map<unsigned long,std::vector<GUID> >::const_iterator& l);
static void AddULong(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,unsigned long v);
static void AddGUID(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const GUID& g);
static void AddString(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,LPCWSTR name,LPCWSTR value);
static void AddPinInfo(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const PININFO& p);
static void AddKSDATARANGE(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,LPCWSTR name,const KSDATARANGE& v);
static void AddKSIDENTIFIER(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const KSIDENTIFIER& v);
static void guid_to_string(REFCLSID rclsid,std::wstring& v);
static void string_to_guid(LPCWSTR v,CLSID* rclsid);
static void GetDeviceList(IXMLDOMElement* e, DEVICELIST& d);
static void GetDevice(IXMLDOMElement* e, DEVICE& d);
static void GetBDATopology(IXMLDOMElement* e,BDATOPOLOGY& v);
static void GetTopology(IXMLDOMElement* e,TOPOLOGY& v);
static void GetPinTopology(IXMLDOMElement* e,PINTOPOLOGY& v);
static void GetBDATemplateConnections(IXMLDOMElement* e,std::vector<BDA_TEMPLATE_CONNECTION>& v);
static void GetBDANodeDescriptors(IXMLDOMElement* e,std::vector<BDANODE_DESCRIPTOR>& v);
static void GetGUIDLists(IXMLDOMElement* e,std::map<unsigned long,std::vector<GUID> >& v);
static void GetULongs(IXMLDOMElement* e,std::vector<unsigned long>& v);
static void GetGUIDs(IXMLDOMElement* e, std::vector<GUID>& v);
static void GetTopologyConnections(IXMLDOMElement* e, std::vector<KSTOPOLOGY_CONNECTION>& v);
static void GetStrings(IXMLDOMElement* e, std::vector<std::wstring>& v);
static void GetPininfos(IXMLDOMElement* e,std::vector<PININFO>& v);
static void GetKSDATARANGE(IXMLDOMElement* e,KSDATARANGE& v);
static void GetKSIDENTIFIER(IXMLDOMElement* e,KSIDENTIFIER& v);


bool SaveXMLDoc(LPCWSTR file, BDADEVICES& d)
{
	IXMLDOMDocumentPtr Document;
	IXMLDOMElementPtr root;
	IXMLDOMElementPtr bda_source;
	IXMLDOMElementPtr bda_reciever;

	HRESULT hr;

	hr = Document.CreateInstance(CLSID_DOMDocument30);
	if(hr==S_OK)
	{
		// these methods should not fail so don't inspect result
		Document->put_async(VARIANT_FALSE);
		Document->put_validateOnParse(VARIANT_FALSE);
		Document->put_resolveExternals(VARIANT_FALSE);

		CreatePI(Document);
		CreateElement(Document,L"bdainf",&root);
		CreateElement(Document,L"kscategory_bda_network_tuner",&bda_source);
		CreateElement(Document,L"kscategory_bda_receiver_component",&bda_reciever);

		CreateDeviceList(Document,bda_source,d.bda_source);
		CreateDeviceList(Document,bda_reciever,d.bda_reciever);

		AppendChild(bda_source,root);
		AppendChild(bda_reciever,root);
		AppendChild(root,Document);

		hr = Document->save(_variant_t(file));
		if(hr==S_OK)
		{
			return true;
		}
		else
		{
			//notify error
			return false;
		}
	}
	else
	{
		//notify error
		return false;
	}
}

bool LoadXMLDoc(LPCWSTR file, BDADEVICES& d)
{
	IXMLDOMDocumentPtr Document;
	HRESULT hr;

	d.reset();
	hr = Document.CreateInstance(CLSID_DOMDocument30);
	if(hr==S_OK)
	{
		VARIANT_BOOL success;
		hr = Document->load(_variant_t(file),&success);
		if(hr==S_OK&&success==VARIANT_TRUE)
		{
			IXMLDOMElementPtr root;
			IXMLDOMElementPtr bda_source;
			IXMLDOMElementPtr bda_reciever;
			IXMLDOMNodePtr bda_source_node;
			IXMLDOMNodePtr bda_reciever_node;

			hr = Document->get_documentElement(&root);
			hr = root->selectSingleNode(_bstr_t(L"kscategory_bda_network_tuner"),&bda_source_node);
			hr = root->selectSingleNode(_bstr_t(L"kscategory_bda_receiver_component"),&bda_reciever_node);

			bda_source = bda_source_node;
			bda_reciever = bda_reciever_node;

			GetDeviceList(bda_source,d.bda_source);
			GetDeviceList(bda_reciever,d.bda_reciever);
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	return false;
}

static void CreatePI(IXMLDOMDocument* pDoc)
{
	HRESULT hr;
	IXMLDOMProcessingInstructionPtr PI;

	hr = pDoc->createProcessingInstruction(L"xml",L"version='1.0' encoding='UTF-8'",&PI);
	if(hr!=S_OK) throw _com_error(hr);
	AppendChild(PI,pDoc);
}

static void AppendChild(IXMLDOMNode* pChild, IXMLDOMNode* pParent)
{
	HRESULT hr;
	IXMLDOMNodePtr out;

	hr = pParent->appendChild(pChild,&out);
	if(hr!=S_OK) throw _com_error(hr);
}

static void CreateElement(IXMLDOMDocument* pDoc, LPCWSTR name,IXMLDOMElement** ppElement)
{
	HRESULT hr;
	hr = pDoc->createElement(_bstr_t(name),ppElement);
	if(hr!=S_OK) throw _com_error(hr);
}

static void CreateDeviceList(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const DEVICELIST& l)
{
	HRESULT hr;
	for(size_t n=0;n<l.size();n++)
	{
		IXMLDOMElementPtr device;
		CreateElement(pDoc,L"device",&device);
		AddDevice(pDoc,device,l[n]);
		AppendChild(device,pParent);
	}
}

static void AddDevice(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const DEVICE& d)
{
	HRESULT hr;
	IXMLDOMElementPtr bdatopology, topology, pintopology;

	AddAttribute(pDoc,L"device_path",d.device_path.c_str(),pParent);
	AddAttribute(pDoc,L"DeviceDesc",d.DeviceDesc.c_str(),pParent);
	AddAttribute(pDoc,L"Service",d.Service.c_str(),pParent);
	AddAttribute(pDoc,L"Class",d.Class.c_str(),pParent);
	AddAttribute(pDoc,L"ClassGUID",d.ClassGUID.c_str(),pParent);
	AddAttribute(pDoc,L"Driver",d.Driver.c_str(),pParent);
	AddAttribute(pDoc,L"PhysicalDeviceObjectName",d.PhysicalDeviceObjectName.c_str(),pParent);
	AddAttribute(pDoc,L"Enumerator_Name",d.Enumerator_Name.c_str(),pParent);
	AddAttribute(pDoc,L"device_instance_id",d.device_instance_id.c_str(),pParent);

	CreateElement(pDoc,L"bdatopology",&bdatopology);
	CreateElement(pDoc,L"topology",&topology);
	CreateElement(pDoc,L"pintopology",&pintopology);

	AddBDATopology(pDoc,bdatopology,d.bdatopology);
	AddTopology(pDoc,topology,d.topology);
	AddPinTopology(pDoc,pintopology,d.pintopology);

	AppendChild(bdatopology,pParent);
	AppendChild(topology,pParent);
	AppendChild(pintopology,pParent);
}

static void AddBDATopology(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const BDATOPOLOGY& t)
{
	HRESULT hr;
	IXMLDOMElementPtr bda_template_connection, bdanode_descriptor, bda_node_properties, bda_node_types, bda_pin_types, bda_node_methods, bda_node_events;

	CreateElement(pDoc,L"ksproperty_bda_template_connections",&bda_template_connection);
	CreateElement(pDoc,L"ksproperty_bda_node_descriptors",&bdanode_descriptor);
	CreateElement(pDoc,L"ksproperty_bda_node_properties",&bda_node_properties);
	CreateElement(pDoc,L"ksproperty_bda_node_types",&bda_node_types);
	CreateElement(pDoc,L"ksproperty_bda_pin_types",&bda_pin_types);
	CreateElement(pDoc,L"ksproperty_bda_node_methods",&bda_node_methods);
	CreateElement(pDoc,L"ksproperty_bda_node_events",&bda_node_events);

	for(size_t n = 0;n<t.bda_template_connection.size();n++)
	{
		AddBDATemplateConnection(pDoc,bda_template_connection,t.bda_template_connection[n]);
	}

	for(size_t n = 0;n<t.bdanode_descriptor.size();n++)
	{
		AddBDANodeDescriptor(pDoc,bdanode_descriptor,t.bdanode_descriptor[n]);
	}

	for(std::map<unsigned long,std::vector<GUID> >::const_iterator it=t.bda_node_properties.begin();it!=t.bda_node_properties.end();it++)
	{
		AddGUIDList(pDoc,bda_node_properties,it);
	}

	for(size_t n = 0;n<t.bda_node_types.size();n++)
	{
		AddULong(pDoc,bda_node_types,t.bda_node_types[n]);
	}

	for(size_t n = 0;n<t.bda_pin_types.size();n++)
	{
		AddULong(pDoc,bda_pin_types,t.bda_pin_types[n]);
	}

	for(std::map<unsigned long,std::vector<GUID> >::const_iterator it=t.bda_node_methods.begin();it!=t.bda_node_methods.end();it++)
	{
		AddGUIDList(pDoc,bda_node_methods,it);
	}

	for(std::map<unsigned long,std::vector<GUID> >::const_iterator it=t.bda_node_events.begin();it!=t.bda_node_events.end();it++)
	{
		AddGUIDList(pDoc,bda_node_events,it);
	}

	AppendChild(bda_template_connection,pParent);
	AppendChild(bdanode_descriptor,pParent);
	AppendChild(bda_node_properties,pParent);
	AppendChild(bda_node_types,pParent);
	AppendChild(bda_pin_types,pParent);
	AppendChild(bda_node_methods,pParent);
	AppendChild(bda_node_events,pParent);
}

static void AddBDATemplateConnection(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent, const BDA_TEMPLATE_CONNECTION& t)
{
	HRESULT hr;
	IXMLDOMElementPtr connection;

	CreateElement(pDoc,L"connection",&connection);
	AddAttribute(pDoc,L"FromNodeType",_variant_t(t.FromNodeType),connection);
	AddAttribute(pDoc,L"FromNodePinType",_variant_t(t.FromNodePinType),connection);
	AddAttribute(pDoc,L"ToNodeType",_variant_t(t.ToNodeType),connection);
	AddAttribute(pDoc,L"ToNodePinType",_variant_t(t.ToNodePinType),connection);
	return AppendChild(connection,pParent);
}

static void AddTopologyConnection(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent, const KSTOPOLOGY_CONNECTION& t)
{
	HRESULT hr;
	IXMLDOMElementPtr connection;

	CreateElement(pDoc,L"connection",&connection);
	AddAttribute(pDoc,L"FromNode",_variant_t(t.FromNode),connection);
	AddAttribute(pDoc,L"FromNodePin",_variant_t(t.FromNodePin),connection);
	AddAttribute(pDoc,L"ToNode",_variant_t(t.ToNode),connection);
	AddAttribute(pDoc,L"ToNodePin",_variant_t(t.ToNodePin),connection);
	return AppendChild(connection,pParent);
}

static void AddBDANodeDescriptor(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent, const BDANODE_DESCRIPTOR& t)
{
	HRESULT hr;
	IXMLDOMElementPtr descriptor;
	std::wstring guid;

	CreateElement(pDoc,L"descriptor",&descriptor);
	AddAttribute(pDoc,L"ulBdaNodeType",_variant_t(t.ulBdaNodeType),descriptor);
	guid_to_string(t.guidFunction,guid);
	AddAttribute(pDoc,L"guidFunction",guid.c_str(),descriptor);
	guid_to_string(t.guidName,guid);
	AddAttribute(pDoc,L"guidName",guid.c_str(),descriptor);

	return AppendChild(descriptor,pParent);
}

static void AddGUIDList(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,std::map<unsigned long,std::vector<GUID> >::const_iterator& l)
{
	HRESULT hr;
	IXMLDOMElementPtr node;

	CreateElement(pDoc,L"node",&node);
	AddAttribute(pDoc,L"id",_variant_t(l->first),node);

	for(size_t n=0;n<l->second.size();n++)
	{
		IXMLDOMElementPtr guid;
		std::wstring guidstr;

		CreateElement(pDoc,L"guid",&guid);
		guid_to_string(l->second[n],guidstr);
		AddAttribute(pDoc,L"name",guidstr.c_str(),guid);
		AppendChild(guid,node);
	}

	return AppendChild(node,pParent);
}

static void AddULong(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,unsigned long v)
{
	HRESULT hr;
	IXMLDOMElementPtr value;

	CreateElement(pDoc,L"value",&value);
	AddAttribute(pDoc,L"val",_variant_t(v),value);
	return AppendChild(value,pParent);
}

static void AddTopology(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const TOPOLOGY& t)
{
	HRESULT hr;
	IXMLDOMElementPtr categories, connection, name, nodes;

	CreateElement(pDoc,L"ksproperty_topology_categories",&categories);
	CreateElement(pDoc,L"ksproperty_topology_connections",&connection);
	CreateElement(pDoc,L"ksproperty_topology_name",&name);
	CreateElement(pDoc,L"ksproperty_topology_nodes",&nodes);

	for(size_t n=0;n<t.categories.size();n++)
	{
		AddGUID(pDoc,categories,t.categories[n]);
	}

	for(size_t n=0;n<t.connection.size();n++)
	{
		AddTopologyConnection(pDoc,connection,t.connection[n]);
	}

	for(size_t n=0;n<t.name.size();n++)
	{
		AddString(pDoc,name,L"name",t.name[n].c_str());
	}

	for(size_t n=0;n<t.nodes.size();n++)
	{
		AddGUID(pDoc,nodes,t.nodes[n]);
	}

	AppendChild(categories,pParent);
	AppendChild(connection,pParent);
	AppendChild(name,pParent);
	AppendChild(nodes,pParent);
}

static void AddGUID(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const GUID& g)
{
	HRESULT hr;
	IXMLDOMElementPtr guid;
	std::wstring guidstr;

	CreateElement(pDoc,L"guid",&guid);
	guid_to_string(g,guidstr);
	AddAttribute(pDoc,L"name",guidstr.c_str(),guid);
	return AppendChild(guid,pParent);
}

static void AddString(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,LPCWSTR elementname,LPCWSTR value)
{
	HRESULT hr;
	IXMLDOMElementPtr name;

	CreateElement(pDoc,elementname,&name);
	AddAttribute(pDoc,L"value",value,name);
	return AppendChild(name,pParent);
}

static void AddPinTopology(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent, const PINTOPOLOGY& t)
{
	HRESULT hr;
	IXMLDOMElementPtr pins;

	AddAttribute(pDoc,L"ksproperty_pin_ctypes",_variant_t(t.pin_ctypes),pParent);
	CreateElement(pDoc,L"pins",&pins);

	for(size_t n=0;n<t.pininfo.size();n++)
	{
		AddPinInfo(pDoc,pins,t.pininfo[n]);
	}

	return AppendChild(pins,pParent);
}


static void AddPinInfo(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const PININFO& p)
{
	HRESULT hr;
	IXMLDOMElementPtr pin;
	CreateElement(pDoc,L"pin",&pin);

	if(p.category_valid)
	{
		std::wstring guid;
		guid_to_string(p.category,guid);
		AddAttribute(pDoc,L"ksproperty_pin_category",guid.c_str(),pin);
	}

	if(p.cinstances_valid)
	{
		IXMLDOMElementPtr cinstances;
		CreateElement(pDoc,L"ksproperty_pin_cinstances",&cinstances);
		AddAttribute(pDoc,L"PossibleCount",_variant_t(p.cinstances.PossibleCount),cinstances);
		AddAttribute(pDoc,L"CurrentCount",_variant_t(p.cinstances.CurrentCount),cinstances);
		AppendChild(cinstances,pin);
	}

	if(p.communication_valid)
	{
		AddAttribute(pDoc,L"ksproperty_pin_communication",_variant_t(p.communication),pin);
	}

	//if(p.constraineddataranges_valid)
	//{
	//}

	if(p.dataflow_valid)
	{
		AddAttribute(pDoc,L"ksproperty_pin_dataflow",_variant_t(p.dataflow),pin);
	}

	if(p.dataintersection_valid)
	{
		AddKSDATARANGE(pDoc,pin,L"ksproperty_pin_dataintersection",p.dataintersection);
	}

	IXMLDOMElementPtr dataranges;
	CreateElement(pDoc,L"ksproperty_pin_dataranges",&dataranges);
	for(size_t n=0;n<p.dataranges.size();n++)
	{
		AddKSDATARANGE(pDoc,dataranges,L"ksdatarange",p.dataranges[n]);
	}
	AppendChild(dataranges,pin);

	if(p.globalcinstances_valid)
	{
		IXMLDOMElementPtr globalcinstances;
		CreateElement(pDoc,L"ksproperty_pin_globalcinstances",&globalcinstances);
		AddAttribute(pDoc,L"PossibleCount",_variant_t(p.globalcinstances.PossibleCount),globalcinstances);
		AddAttribute(pDoc,L"CurrentCount",_variant_t(p.globalcinstances.CurrentCount),globalcinstances);
		AppendChild(globalcinstances,pin);
	}

	IXMLDOMElementPtr interfaces;
	CreateElement(pDoc,L"ksproperty_pin_interfaces",&interfaces);
	for(size_t n=0;n<p.interfaces.size();n++)
	{
		AddKSIDENTIFIER(pDoc,interfaces,p.interfaces[n]);
	}
	AppendChild(interfaces,pin);

	IXMLDOMElementPtr mediums;
	CreateElement(pDoc,L"ksproperty_pin_mediums",&mediums);
	for(size_t n=0;n<p.mediums.size();n++)
	{
		AddKSIDENTIFIER(pDoc,interfaces,p.mediums[n]);
	}
	AppendChild(mediums,pin);

	AddAttribute(pDoc,L"ksproperty_pin_name",p.name.c_str(),pin);

	if(p.necessaryinstances_valid)
	{
		AddAttribute(pDoc,L"ksproperty_pin_necessaryinstances",_variant_t(p.necessaryinstances),pin);
	}

	if(p.physicalconnection_valid)
	{
		IXMLDOMElementPtr physicalconnection;
		CreateElement(pDoc,L"ksproperty_pin_physicalconnection",&physicalconnection);
		AddAttribute(pDoc,L"Size",_variant_t(p.physicalconnection.Size),physicalconnection);
		AddAttribute(pDoc,L"Pin",_variant_t(p.physicalconnection.Pin),physicalconnection);
		AddAttribute(pDoc,L"SymbolicLinkName",p.physicalconnection.SymbolicLinkName,physicalconnection);
		AppendChild(physicalconnection,pin);
	}

	return AppendChild(pin,pParent);
}

static void AddKSDATARANGE(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,LPCWSTR name,const KSDATARANGE& v)
{
	HRESULT hr;
	IXMLDOMElementPtr element;
	std::wstring guid;
	CreateElement(pDoc,name,&element);
	AddAttribute(pDoc,L"FormatSize",_variant_t(v.FormatSize),element);
	AddAttribute(pDoc,L"Flags",_variant_t(v.Flags),element);
	AddAttribute(pDoc,L"SampleSize",_variant_t(v.SampleSize),element);
	AddAttribute(pDoc,L"Reserved",_variant_t(v.Reserved),element);
	guid_to_string(v.MajorFormat,guid);
	AddAttribute(pDoc,L"MajorFormat",guid.c_str(),element);
	guid_to_string(v.SubFormat,guid);
	AddAttribute(pDoc,L"SubFormat",guid.c_str(),element);
	guid_to_string(v.Specifier,guid);
	AddAttribute(pDoc,L"Specifier",guid.c_str(),element);
	return AppendChild(element,pParent);
}

static void AddKSIDENTIFIER(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const KSIDENTIFIER& v)
{
	HRESULT hr;
	IXMLDOMElementPtr element;
	std::wstring guid;

	CreateElement(pDoc,L"ksidentifier",&element);
	guid_to_string(v.Set,guid);
	AddAttribute(pDoc,L"Set",guid.c_str(),element);
	AddAttribute(pDoc,L"Id",_variant_t(v.Id),element);
	AddAttribute(pDoc,L"Flags",_variant_t(v.Flags),element);
	return AppendChild(element,pParent);
}

static void AddAttribute(IXMLDOMDocument* pDoc,LPCWSTR name, LPCWSTR value,IXMLDOMElement* pParent)
{
	HRESULT hr;
	IXMLDOMAttributePtr Attribute, AttributeOut;

	hr = pDoc->createAttribute(_bstr_t(name),&Attribute);
	if(hr!=S_OK) throw _com_error(hr);
	hr = Attribute->put_value(_variant_t(value));
	if(hr!=S_OK) throw _com_error(hr);
	hr = pParent->setAttributeNode(Attribute,&AttributeOut);
	if(hr!=S_OK) throw _com_error(hr);
}

static void AddAttribute(IXMLDOMDocument* pDoc,LPCWSTR name, _variant_t& value,IXMLDOMElement* pParent)
{
	HRESULT hr;
	IXMLDOMAttributePtr Attribute, AttributeOut;

	hr = pDoc->createAttribute(_bstr_t(name),&Attribute);
	if(hr!=S_OK) throw _com_error(hr);
	hr = Attribute->put_value(value);
	if(hr!=S_OK) throw _com_error(hr);
	hr = pParent->setAttributeNode(Attribute,&AttributeOut);
	if(hr!=S_OK) throw _com_error(hr);
}

static void guid_to_string(REFCLSID rclsid,std::wstring& v)
{
	HRESULT hr;
	RPC_WSTR lpolestr;
	if((hr=UuidToString(&rclsid,&lpolestr ))==S_OK)
	{
		v=(LPCWSTR)lpolestr;
		RpcStringFree (&lpolestr);
	}
	else
	{
		if(hr!=S_OK) throw _com_error(hr);
	}
}

static void string_to_guid(LPCWSTR v,CLSID* rclsid)
{
	HRESULT hr;
	hr = UuidFromString((RPC_WSTR)v,rclsid);
	if(hr!=S_OK) throw _com_error(hr);
}

static void GetDeviceList(IXMLDOMElement* e, DEVICELIST& d)
{
	HRESULT hr;
	IXMLDOMNodeListPtr list;
	long listlength;

	hr = e->get_childNodes(&list);
	if(hr!=S_OK) throw _com_error(hr);

	hr = list->get_length(&listlength);
	if(hr!=S_OK) throw _com_error(hr);
	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMNodePtr itemnode;
		IXMLDOMElementPtr itemlement;

		hr = list->get_item(n,&itemnode);
		if(hr!=S_OK) throw _com_error(hr);
		itemlement = itemnode;

		DEVICE device;
		GetDevice(itemlement,device);
		d.push_back(device);

	}
}

static void GetDevice(IXMLDOMElement* e, DEVICE& d)
{
	HRESULT hr;
	_variant_t v;
	//IXMLDOMNamedNodeMapPtr attributes;
	IXMLDOMNodePtr bdatopologynode;
	IXMLDOMElementPtr bdatopology;
	IXMLDOMNodePtr topologynode;
	IXMLDOMElementPtr topology;
	IXMLDOMNodePtr pintopologynode;
	IXMLDOMElementPtr pintopology;

	hr = e->getAttribute(_bstr_t("device_path"),&v);
	if(hr!=S_OK) throw _com_error(hr);
	d.device_path = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	hr = e->getAttribute(_bstr_t("DeviceDesc"),&v);
	if(hr!=S_OK) throw _com_error(hr);
	d.DeviceDesc = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	hr = e->getAttribute(_bstr_t("Service"),&v);
	if(hr!=S_OK) throw _com_error(hr);
	d.Service = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	hr = e->getAttribute(_bstr_t("Class"),&v);
	if(hr!=S_OK) throw _com_error(hr);
	d.Class = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	hr = e->getAttribute(_bstr_t("ClassGUID"),&v);
	if(hr!=S_OK) throw _com_error(hr);
	d.ClassGUID = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	hr = e->getAttribute(_bstr_t("Driver"),&v);
	if(hr!=S_OK) throw _com_error(hr);
	d.Driver = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	hr = e->getAttribute(_bstr_t("PhysicalDeviceObjectName"),&v);
	if(hr!=S_OK) throw _com_error(hr);
	d.PhysicalDeviceObjectName = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	hr = e->getAttribute(_bstr_t("Enumerator_Name"),&v);
	if(hr!=S_OK) throw _com_error(hr);
	d.Enumerator_Name = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	hr = e->getAttribute(_bstr_t("device_instance_id"),&v);
	if(hr!=S_OK) throw _com_error(hr);
	d.device_instance_id = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	
	hr = e->selectSingleNode(_bstr_t(L"bdatopology"),&bdatopologynode);
	if(hr!=S_OK) throw _com_error(hr);
	bdatopology = bdatopologynode;
	hr = e->selectSingleNode(_bstr_t(L"topology"),&topologynode);
	if(hr!=S_OK) throw _com_error(hr);
	topology = topologynode;
	hr = e->selectSingleNode(_bstr_t(L"pintopology"),&pintopologynode);
	if(hr!=S_OK) throw _com_error(hr);
	pintopology = pintopologynode;

	GetBDATopology(bdatopology, d.bdatopology);
	GetTopology(topology, d.topology);
	GetPinTopology(pintopology,d.pintopology);
}

static void GetBDATopology(IXMLDOMElement* e,BDATOPOLOGY& v)
{
	HRESULT hr;
	IXMLDOMElementPtr bda_template_connection, bdanode_descriptor, bda_node_properties, bda_node_types, bda_pin_types, bda_node_methods, bda_node_events;
	IXMLDOMNodePtr bda_template_connection_node, bdanode_descriptor_node, bda_node_properties_node, bda_node_types_node, bda_pin_types_node, bda_node_methods_node, bda_node_events_node;

	hr = e->selectSingleNode(_bstr_t(L"ksproperty_bda_template_connections"),&bda_template_connection_node);
	if(hr!=S_OK) throw _com_error(hr);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_bda_node_descriptors"),&bdanode_descriptor_node);
	if(hr!=S_OK) throw _com_error(hr);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_bda_node_properties"),&bda_node_properties_node);
	if(hr!=S_OK) throw _com_error(hr);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_bda_node_types"),&bda_node_types_node);
	if(hr!=S_OK) throw _com_error(hr);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_bda_pin_types"),&bda_pin_types_node);
	if(hr!=S_OK) throw _com_error(hr);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_bda_node_methods"),&bda_node_methods_node);
	if(hr!=S_OK) throw _com_error(hr);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_bda_node_events"),&bda_node_events_node);
	if(hr!=S_OK) throw _com_error(hr);

	bda_template_connection=bda_template_connection_node;
	bdanode_descriptor=bdanode_descriptor_node;
	bda_node_properties=bda_node_properties_node;
	bda_node_types=bda_node_types_node;
	bda_pin_types=bda_pin_types_node;
	bda_node_methods=bda_node_methods_node;
	bda_node_events=bda_node_events_node;

	GetBDATemplateConnections(bda_template_connection,v.bda_template_connection);
	GetBDANodeDescriptors(bdanode_descriptor, v.bdanode_descriptor);
	GetGUIDLists(bda_node_properties, v.bda_node_properties);
	GetULongs(bda_node_types, v.bda_node_types);
	GetULongs(bda_pin_types, v.bda_pin_types);
	GetGUIDLists(bda_node_methods, v.bda_node_methods);
	GetGUIDLists(bda_node_events, v.bda_node_events);
}

static void GetTopology(IXMLDOMElement* e,TOPOLOGY& v)
{
	HRESULT hr;
	IXMLDOMElementPtr categories, connection, name, nodes;
	IXMLDOMNodePtr  categories_node, connection_node, name_node, nodes_node;

	hr = e->selectSingleNode(_bstr_t(L"ksproperty_topology_categories"),&categories_node);
	if(hr!=S_OK) throw _com_error(hr);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_topology_connections"),&connection_node);
	if(hr!=S_OK) throw _com_error(hr);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_topology_name"),&name_node);
	if(hr!=S_OK) throw _com_error(hr);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_topology_nodes"),&nodes_node);
	if(hr!=S_OK) throw _com_error(hr);

	categories=categories_node;
	connection=connection_node;
	name=name_node;
	nodes=nodes_node;

	GetGUIDs(categories,v.categories);
	GetTopologyConnections(connection,v.connection);
	GetStrings(name,v.name);
	GetGUIDs(nodes,v.nodes);
}

static void GetPinTopology(IXMLDOMElement* e,PINTOPOLOGY& v)
{
	HRESULT hr;
	_variant_t var;
	IXMLDOMElementPtr pins;
	IXMLDOMNodePtr pins_node;

	hr = e->getAttribute(_bstr_t(L"ksproperty_pin_ctypes"),&var);
	if(hr!=S_OK) throw _com_error(hr);
	v.pin_ctypes = (unsigned long)var;

	hr = e->selectSingleNode(_bstr_t(L"pins"),&pins_node);
	if(hr!=S_OK) throw _com_error(hr);
	pins = pins_node;

	GetPininfos(pins,v.pininfo);
}

static void GetBDATemplateConnections(IXMLDOMElement* e,std::vector<BDA_TEMPLATE_CONNECTION>& v)
{
	HRESULT hr;
	IXMLDOMNodeListPtr children;
	long listlength;

	hr = e->get_childNodes(&children);
	if(hr!=S_OK) throw _com_error(hr);
	hr = children->get_length(&listlength);
	if(hr!=S_OK) throw _com_error(hr);

	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMElementPtr item;
		IXMLDOMNodePtr itemnode;
		_variant_t var;
		BDA_TEMPLATE_CONNECTION con;

		hr = children->get_item(n,&itemnode);
		if(hr!=S_OK) throw _com_error(hr);
		item = itemnode;

		hr = item->getAttribute(_bstr_t(L"FromNodeType"),&var);
		if(hr!=S_OK) throw _com_error(hr);
		con.FromNodeType = (unsigned long)var;
		var.Clear();
		hr = item->getAttribute(_bstr_t(L"FromNodePinType"),&var);
		if(hr!=S_OK) throw _com_error(hr);
		con.FromNodePinType = (unsigned long)var;
		var.Clear();
		hr = item->getAttribute(_bstr_t(L"ToNodeType"),&var);
		if(hr!=S_OK) throw _com_error(hr);
		con.ToNodeType = (unsigned long)var;
		var.Clear();
		hr = item->getAttribute(_bstr_t(L"ToNodePinType"),&var);
		if(hr!=S_OK) throw _com_error(hr);
		con.ToNodePinType = (unsigned long)var;
		var.Clear();
		v.push_back(con);
	}
}

static void GetBDANodeDescriptors(IXMLDOMElement* e,std::vector<BDANODE_DESCRIPTOR>& v)
{
	HRESULT hr;
	IXMLDOMNodeListPtr children;
	long listlength;

	hr = e->get_childNodes(&children);
	if(hr!=S_OK) throw _com_error(hr);
	hr = children->get_length(&listlength);
	if(hr!=S_OK) throw _com_error(hr);

	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMElementPtr item;
		IXMLDOMNodePtr itemnode;
		_variant_t var;

		BDANODE_DESCRIPTOR con;

		hr = children->get_item(n,&itemnode);
		if(hr!=S_OK) throw _com_error(hr);
		item = itemnode;

		hr = item->getAttribute(_bstr_t(L"ulBdaNodeType"),&var);
		if(hr!=S_OK) throw _com_error(hr);
		con.ulBdaNodeType = (unsigned long)var;
		var.Clear();
		hr = item->getAttribute(_bstr_t(L"guidFunction"),&var);
		if(hr!=S_OK) throw _com_error(hr);
		string_to_guid((LPCWSTR)(_bstr_t)var,&con.guidFunction);
		var.Clear();
		hr = item->getAttribute(_bstr_t(L"guidName"),&var);
		if(hr!=S_OK) throw _com_error(hr);
		string_to_guid((LPCWSTR)(_bstr_t)var,&con.guidName);
		var.Clear();
	}
}

static void GetGUIDLists(IXMLDOMElement* e,std::map<unsigned long,std::vector<GUID> >& v)
{
	HRESULT hr;
	IXMLDOMNodeListPtr children;
	long listlength;

	hr = e->get_childNodes(&children);
	if(hr!=S_OK) throw _com_error(hr);
	hr = children->get_length(&listlength);
	if(hr!=S_OK) throw _com_error(hr);

	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMElementPtr item;
		IXMLDOMNodePtr itemnode;
		_variant_t var;

		hr = children->get_item(n,&itemnode);
		if(hr!=S_OK) throw _com_error(hr);
		item = itemnode;

		hr = item->getAttribute(_bstr_t(L"id"),&var);
		if(hr!=S_OK) throw _com_error(hr);
		GetGUIDs(item,v[(unsigned long)var]);

	}
}

static void GetULongs(IXMLDOMElement* e,std::vector<unsigned long>& v)
{
	HRESULT hr;
	IXMLDOMNodeListPtr children;
	long listlength;

	hr = e->get_childNodes(&children);
	if(hr!=S_OK) throw _com_error(hr);
	hr = children->get_length(&listlength);
	if(hr!=S_OK) throw _com_error(hr);

	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMElementPtr item;
		IXMLDOMNodePtr itemnode;
		_variant_t var;
		unsigned long val;

		hr = children->get_item(n,&itemnode);
		if(hr!=S_OK) throw _com_error(hr);
		item = itemnode;

		hr = item->getAttribute(_bstr_t(L"val"),&var);
		if(hr!=S_OK) throw _com_error(hr);
		val = (unsigned long)var;
		v.push_back(val);
	}
}

static void GetGUIDs(IXMLDOMElement* e, std::vector<GUID>& v)
{
	HRESULT hr;
	IXMLDOMNodeListPtr children;
	long listlength;

	hr = e->get_childNodes(&children);
	if(hr!=S_OK) throw _com_error(hr);
	hr = children->get_length(&listlength);
	if(hr!=S_OK) throw _com_error(hr);

	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMElementPtr item;
		IXMLDOMNodePtr itemnode;
		_variant_t var;
		GUID guid;

		hr = children->get_item(n,&itemnode);
		if(hr!=S_OK) throw _com_error(hr);
		item = itemnode;

		hr = item->getAttribute(_bstr_t(L"name"),&var);
		if(hr!=S_OK) throw _com_error(hr);
		string_to_guid((LPCWSTR)(_bstr_t)var,&guid);
		v.push_back(guid);
	}
}

static void GetTopologyConnections(IXMLDOMElement* e, std::vector<KSTOPOLOGY_CONNECTION>& v)
{
	HRESULT hr;
	IXMLDOMNodeListPtr children;
	long listlength;

	hr = e->get_childNodes(&children);
	if(hr!=S_OK) throw _com_error(hr);
	hr = children->get_length(&listlength);
	if(hr!=S_OK) throw _com_error(hr);

	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMElementPtr item;
		IXMLDOMNodePtr itemnode;
		_variant_t var;
		KSTOPOLOGY_CONNECTION con;

		hr = children->get_item(n,&itemnode);
		item = itemnode;

		hr = item->getAttribute(_bstr_t(L"FromNode"),&var);
		if(hr!=S_OK) throw _com_error(hr);
		con.FromNode = (unsigned long)var;
		var.Clear();
		hr = item->getAttribute(_bstr_t(L"FromNodePin"),&var);
		if(hr!=S_OK) throw _com_error(hr);
		con.FromNodePin = (unsigned long)var;
		var.Clear();
		hr = item->getAttribute(_bstr_t(L"ToNode"),&var);
		if(hr!=S_OK) throw _com_error(hr);
		con.ToNode = (unsigned long)var;
		var.Clear();
		hr = item->getAttribute(_bstr_t(L"ToNodePin"),&var);
		if(hr!=S_OK) throw _com_error(hr);
		con.ToNodePin = (unsigned long)var;
		var.Clear();
		v.push_back(con);
	}
}

static void GetStrings(IXMLDOMElement* e, std::vector<std::wstring>& v)
{
	HRESULT hr;
	IXMLDOMNodeListPtr children;
	long listlength;

	hr = e->get_childNodes(&children);
	if(hr!=S_OK) throw _com_error(hr);
	hr = children->get_length(&listlength);
	if(hr!=S_OK) throw _com_error(hr);

	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMElementPtr item;
		IXMLDOMNodePtr itemnode;
		_variant_t var;
		std::wstring str;

		hr = children->get_item(n,&itemnode);
		if(hr!=S_OK) throw _com_error(hr);
		item = itemnode;

		hr = item->getAttribute(_bstr_t(L"value"),&var);
		if(hr!=S_OK) throw _com_error(hr);
		str = (LPCWSTR)(_bstr_t)var;
		v.push_back(str);
	}
}

static void GetPininfos(IXMLDOMElement* e,std::vector<PININFO>& v)
{
	HRESULT hr;
	IXMLDOMNodeListPtr children;
	long listlength;

	hr = e->get_childNodes(&children);
	if(hr!=S_OK) throw _com_error(hr);
	hr = children->get_length(&listlength);
	if(hr!=S_OK) throw _com_error(hr);

	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMElementPtr item, item2;
		IXMLDOMNodePtr itemnode, itemnode2;
		_variant_t var;
		PININFO pininfo;

		hr = children->get_item(n,&itemnode);
		if(hr!=S_OK) throw _com_error(hr);
		item = itemnode;

		hr = item->getAttribute(_bstr_t(L"ksproperty_pin_category"),&var);
		if(hr==S_OK)
		{
			string_to_guid((LPCWSTR)(_bstr_t)var,&pininfo.category);
			pininfo.category_valid = true;
			var.Clear();
		}

		hr = item->selectSingleNode(_bstr_t(L"ksproperty_pin_cinstances"),&itemnode2);
		if(hr==S_OK)
		{
			item2 = itemnode2;
			hr = item2->getAttribute(_bstr_t(L"PossibleCount"),&var);
			if(hr!=S_OK) throw _com_error(hr);
			pininfo.cinstances.PossibleCount = (unsigned long)var;
			var.Clear();
			hr = item2->getAttribute(_bstr_t(L"CurrentCount"),&var);
			if(hr!=S_OK) throw _com_error(hr);
			pininfo.cinstances.CurrentCount = (unsigned long)var;
			var.Clear();
			pininfo.cinstances_valid = true;
			itemnode2.Release();
			item2.Release();
		}

		hr = item->getAttribute(_bstr_t(L"ksproperty_pin_communication"),&var);
		if(hr==S_OK)
		{
			pininfo.communication = (KSPIN_COMMUNICATION)(unsigned long)var;
			pininfo.communication_valid = true;
			var.Clear();
		}

		hr = item->getAttribute(_bstr_t(L"ksproperty_pin_dataflow"),&var);
		if(hr==S_OK)
		{
			pininfo.dataflow = (KSPIN_DATAFLOW)(unsigned long)var;
			pininfo.dataflow_valid = true;
			var.Clear();
		}

		hr = item->selectSingleNode(_bstr_t(L"ksproperty_pin_dataintersection"),&itemnode2);
		if(hr == S_OK)
		{
			item2 = itemnode2;
			GetKSDATARANGE(item2,pininfo.dataintersection);
			pininfo.dataintersection_valid = true;
			itemnode2.Release();
			item2.Release();
		}

		hr = item->selectSingleNode(_bstr_t(L"ksproperty_pin_dataranges"),&itemnode2);
		if(hr==S_OK)
		{
			IXMLDOMNodeListPtr children2;
			long listlength2;
			item2 = itemnode2;

			hr = item2->get_childNodes(&children2);
			if(hr!=S_OK) throw _com_error(hr);
			hr = children2->get_length(&listlength2);
			if(hr!=S_OK) throw _com_error(hr);

			for(size_t m=0;m<listlength2;m++)
			{
				IXMLDOMElementPtr item3;
				IXMLDOMNodePtr itemnode3;
				KSDATARANGE ksdatarange;

				hr = children2->get_item(m, &itemnode3);
				if(hr!=S_OK) throw _com_error(hr);
				item3 = itemnode3;

				GetKSDATARANGE(item3,ksdatarange);
				pininfo.dataranges.push_back(ksdatarange);
			}

			item2.Release();
			itemnode2.Release();
		}

		hr = item->selectSingleNode(_bstr_t(L"ksproperty_pin_globalcinstances"),&itemnode2);
		if(hr==S_OK)
		{
			item2 = itemnode2;
			hr = item2->getAttribute(_bstr_t(L"PossibleCount"),&var);
			if(hr!=S_OK) throw _com_error(hr);
			pininfo.globalcinstances.PossibleCount = (unsigned long)var;
			var.Clear();
			hr = item2->getAttribute(_bstr_t(L"CurrentCount"),&var);
			if(hr!=S_OK) throw _com_error(hr);
			pininfo.globalcinstances.CurrentCount = (unsigned long)var;
			var.Clear();
			pininfo.globalcinstances_valid = true;
			itemnode2.Release();
			item2.Release();
		}

		hr = item->selectSingleNode(_bstr_t(L"ksproperty_pin_interfaces"),&itemnode2);
		if(hr==S_OK)
		{
			IXMLDOMNodeListPtr children2;
			long listlength2;
			item2 = itemnode2;

			hr = item2->get_childNodes(&children2);
			if(hr!=S_OK) throw _com_error(hr);
			hr = children2->get_length(&listlength2);
			if(hr!=S_OK) throw _com_error(hr);

			for(size_t m=0;m<listlength2;m++)
			{
				IXMLDOMElementPtr item3;
				IXMLDOMNodePtr itemnode3;
				KSIDENTIFIER ksidentifier;

				hr = children2->get_item(m, &itemnode3);
				if(hr!=S_OK) throw _com_error(hr);
				item3 = itemnode3;

				GetKSIDENTIFIER(item3,ksidentifier);
				pininfo.interfaces.push_back(ksidentifier);
			}

			itemnode2.Release();
			item2.Release();
		}

		hr = item->selectSingleNode(_bstr_t(L"ksproperty_pin_mediums"),&itemnode2);
		if(hr==S_OK)
		{
			IXMLDOMNodeListPtr children2;
			long listlength2;
			item2 = itemnode2;

			hr = item2->get_childNodes(&children2);
			if(hr!=S_OK) throw _com_error(hr);
			hr = children2->get_length(&listlength2);
			if(hr!=S_OK) throw _com_error(hr);

			for(size_t m=0;m<listlength2;m++)
			{
				IXMLDOMElementPtr item3;
				IXMLDOMNodePtr itemnode3;
				KSIDENTIFIER ksidentifier;

				hr = children2->get_item(m, &itemnode3);
				if(hr!=S_OK) throw _com_error(hr);
				item3 = itemnode3;

				GetKSIDENTIFIER(item3,ksidentifier);
				pininfo.mediums.push_back(ksidentifier);
			}

			itemnode2.Release();
			item2.Release();
		}

		hr = item->getAttribute(_bstr_t(L"ksproperty_pin_name"),&var);
		if(hr==S_OK)
		{
			pininfo.name = (LPCWSTR)(_bstr_t)var;
			var.Clear();
		}

		hr = item->getAttribute(_bstr_t(L"ksproperty_pin_necessaryinstances"),&var);
		if(hr==S_OK)
		{
			pininfo.necessaryinstances = (unsigned long)var;
			pininfo.necessaryinstances_valid = true;
			var.Clear();
		}

		hr = item->selectSingleNode(_bstr_t(L"ksproperty_pin_physicalconnection"),&itemnode2);
		if(hr==S_OK)
		{
			item2 = itemnode2;
			hr = item2->getAttribute(_bstr_t(L"Size"),&var);
			if(hr!=S_OK) throw _com_error(hr);
			pininfo.physicalconnection.Size = (unsigned long)var;
			var.Clear();
			hr = item2->getAttribute(_bstr_t(L"Pin"),&var);
			if(hr!=S_OK) throw _com_error(hr);
			pininfo.physicalconnection.Pin = (unsigned long)var;
			var.Clear();
			hr = item2->getAttribute(_bstr_t(L"SymbolicLinkName"),&var);
			if(hr!=S_OK) throw _com_error(hr);
			pininfo.symboliclinkname = (LPCWSTR)(_bstr_t)var;
			var.Clear();
			pininfo.physicalconnection_valid = true;
			itemnode2.Release();
			item2.Release();
		}

		v.push_back(pininfo);
	}
}

static void GetKSDATARANGE(IXMLDOMElement* e,KSDATARANGE& v)
{
	HRESULT hr;
	_variant_t var;

	hr = e->getAttribute(_bstr_t(L"FormatSize"),&var);
	if(hr!=S_OK) throw _com_error(hr);
	v.FormatSize = (unsigned long)var;
	var.Clear();
	hr = e->getAttribute(_bstr_t(L"Flags"),&var);
	if(hr!=S_OK) throw _com_error(hr);
	v.Flags = (unsigned long)var;
	var.Clear();
	hr = e->getAttribute(_bstr_t(L"SampleSize"),&var);
	if(hr!=S_OK) throw _com_error(hr);
	v.SampleSize = (unsigned long)var;
	var.Clear();
	hr = e->getAttribute(_bstr_t(L"Reserved"),&var);
	if(hr!=S_OK) throw _com_error(hr);
	v.Reserved = (unsigned long)var;
	var.Clear();
	hr = e->getAttribute(_bstr_t(L"MajorFormat"),&var);
	if(hr!=S_OK) throw _com_error(hr);
	string_to_guid((LPCWSTR)(_bstr_t)var,&v.MajorFormat);
	var.Clear();
	hr = e->getAttribute(_bstr_t(L"SubFormat"),&var);
	if(hr!=S_OK) throw _com_error(hr);
	string_to_guid((LPCWSTR)(_bstr_t)var,&v.SubFormat);
	var.Clear();
	hr = e->getAttribute(_bstr_t(L"Specifier"),&var);
	if(hr!=S_OK) throw _com_error(hr);
	string_to_guid((LPCWSTR)(_bstr_t)var,&v.Specifier);
	var.Clear();
}

static void GetKSIDENTIFIER(IXMLDOMElement* e,KSIDENTIFIER& v)
{
	HRESULT hr;
	_variant_t var;

	hr = e->getAttribute(_bstr_t(L"Set"),&var);
	if(hr!=S_OK) throw _com_error(hr);
	string_to_guid((LPCWSTR)(_bstr_t)var,&v.Set);
	var.Clear();
	hr = e->getAttribute(_bstr_t(L"Id"),&var);
	if(hr!=S_OK) throw _com_error(hr);
	v.Id = (unsigned long)var;
	var.Clear();
	hr = e->getAttribute(_bstr_t(L"Flags"),&var);
	if(hr!=S_OK) throw _com_error(hr);
	v.Flags = (unsigned long)var;
	var.Clear();
}
