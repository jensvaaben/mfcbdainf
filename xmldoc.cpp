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

static HRESULT CreatePI(IXMLDOMDocument* pDoc);
static HRESULT AppendChild(IXMLDOMNode* pChild, IXMLDOMNode* pParent);
static HRESULT CreateElement(IXMLDOMDocument* pDoc, LPCWSTR name,IXMLDOMElement** ppElement);
static HRESULT CreateDeviceList(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const DEVICELIST& l);
static HRESULT AddDevice(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const DEVICE& d);
static HRESULT AddAttribute(IXMLDOMDocument* pDoc,LPCWSTR name, LPCWSTR value,IXMLDOMElement* pParent);
static HRESULT AddAttribute(IXMLDOMDocument* pDoc,LPCWSTR name, _variant_t& v,IXMLDOMElement* pParent);
static HRESULT AddBDATopology(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const BDATOPOLOGY& t);
static HRESULT AddTopology(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const TOPOLOGY& t);
static HRESULT AddPinTopology(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const PINTOPOLOGY& t);
static HRESULT AddBDATemplateConnection(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent, const BDA_TEMPLATE_CONNECTION& t);
static HRESULT AddTopologyConnection(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent, const KSTOPOLOGY_CONNECTION& t);
static HRESULT AddBDANodeDescriptor(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent, const BDANODE_DESCRIPTOR& t);
static HRESULT AddGUIDList(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,std::map<unsigned long,std::vector<GUID> >::const_iterator& l);
static HRESULT AddULong(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,unsigned long v);
static HRESULT AddGUID(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const GUID& g);
static HRESULT AddString(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,LPCWSTR name,LPCWSTR value);
static HRESULT AddPinInfo(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const PININFO& p);
static HRESULT AddKSDATARANGE(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,LPCWSTR name,const KSDATARANGE& v);
static HRESULT guid_to_string(REFCLSID rclsid,std::wstring& v);
static HRESULT string_to_guid(LPCWSTR v,CLSID* rclsid);
static HRESULT GetDeviceList(IXMLDOMElement* e, DEVICELIST& d);
static HRESULT GetDevice(IXMLDOMElement* e, DEVICE& d);
static HRESULT GetBDATopology(IXMLDOMElement* e,BDATOPOLOGY& v);
static HRESULT GetTopology(IXMLDOMElement* e,TOPOLOGY& v);
static HRESULT GetPinTopology(IXMLDOMElement* e,PINTOPOLOGY& v);
static HRESULT GetBDATemplateConnections(IXMLDOMElement* e,std::vector<BDA_TEMPLATE_CONNECTION>& v);
static HRESULT GetBDANodeDescriptors(IXMLDOMElement* e,std::vector<BDANODE_DESCRIPTOR>& v);
static HRESULT GetGUIDLists(IXMLDOMElement* e,std::map<unsigned long,std::vector<GUID> >& v);
static HRESULT GetULongs(IXMLDOMElement* e,std::vector<unsigned long>& v);
static HRESULT GetGUIDs(IXMLDOMElement* e, std::vector<GUID>& v);
static HRESULT GetTopologyConnections(IXMLDOMElement* e, std::vector<KSTOPOLOGY_CONNECTION>& v);
static HRESULT GetStrings(IXMLDOMElement* e, std::vector<std::wstring>& v);
static HRESULT GetPininfos(IXMLDOMElement* e,std::vector<PININFO>& v);
static HRESULT GetKSDATARANGE(IXMLDOMElement* e,KSDATARANGE& v);
static HRESULT GetKSIDENTIFIER(IXMLDOMElement* e,KSIDENTIFIER& v);


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

		hr = CreatePI(Document);
		hr = CreateElement(Document,L"bdainf",&root);
		hr = CreateElement(Document,L"kscategory_bda_network_tuner",&bda_source);
		hr = CreateElement(Document,L"kscategory_bda_receiver_component",&bda_reciever);

		CreateDeviceList(Document,bda_source,d.bda_source);
		CreateDeviceList(Document,bda_reciever,d.bda_reciever);

		hr = AppendChild(bda_source,root);
		hr = AppendChild(bda_reciever,root);
		hr = AppendChild(root,Document);

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

			hr = GetDeviceList(bda_source,d.bda_source);
			hr = GetDeviceList(bda_reciever,d.bda_reciever);
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

static HRESULT CreatePI(IXMLDOMDocument* pDoc)
{
	HRESULT hr;
	IXMLDOMProcessingInstructionPtr PI;

	hr = pDoc->createProcessingInstruction(L"xml",L"version='1.0' encoding='UTF-8'",&PI);
	if(hr==S_OK)
	{
		return AppendChild(PI,pDoc);
	}
	else
	{
		//notify error
		return hr;
	}

	return S_OK;
}

static HRESULT AppendChild(IXMLDOMNode* pChild, IXMLDOMNode* pParent)
{
	IXMLDOMNodePtr out;

	return pParent->appendChild(pChild,&out);
}

static HRESULT CreateElement(IXMLDOMDocument* pDoc, LPCWSTR name,IXMLDOMElement** ppElement)
{
	return pDoc->createElement(_bstr_t(name),ppElement);
}

static HRESULT CreateDeviceList(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const DEVICELIST& l)
{
	HRESULT hr;
	for(size_t n=0;n<l.size();n++)
	{
		IXMLDOMElementPtr device;
		hr = CreateElement(pDoc,L"device",&device);
		hr = AddDevice(pDoc,device,l[n]);
		hr = AppendChild(device,pParent);
	}

	return S_OK;
}

static HRESULT AddDevice(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const DEVICE& d)
{
	HRESULT hr;
	IXMLDOMElementPtr bdatopology, topology, pintopology;

	hr = AddAttribute(pDoc,L"device_path",d.device_path.c_str(),pParent);
	hr = AddAttribute(pDoc,L"DeviceDesc",d.DeviceDesc.c_str(),pParent);
	hr = AddAttribute(pDoc,L"Service",d.Service.c_str(),pParent);
	hr = AddAttribute(pDoc,L"Class",d.Class.c_str(),pParent);
	hr = AddAttribute(pDoc,L"ClassGUID",d.ClassGUID.c_str(),pParent);
	hr = AddAttribute(pDoc,L"Driver",d.Driver.c_str(),pParent);
	hr = AddAttribute(pDoc,L"PhysicalDeviceObjectName",d.PhysicalDeviceObjectName.c_str(),pParent);
	hr = AddAttribute(pDoc,L"Enumerator_Name",d.Enumerator_Name.c_str(),pParent);
	hr = AddAttribute(pDoc,L"device_instance_id",d.device_instance_id.c_str(),pParent);

	hr = CreateElement(pDoc,L"bdatopology",&bdatopology);
	hr = CreateElement(pDoc,L"topology",&topology);
	hr = CreateElement(pDoc,L"pintopology",&pintopology);

	hr = AddBDATopology(pDoc,bdatopology,d.bdatopology);
	hr = AddTopology(pDoc,topology,d.topology);
	hr = AddPinTopology(pDoc,pintopology,d.pintopology);

	hr = AppendChild(bdatopology,pParent);
	hr = AppendChild(topology,pParent);
	hr = AppendChild(pintopology,pParent);

	return S_OK;
}

static HRESULT AddBDATopology(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const BDATOPOLOGY& t)
{
	HRESULT hr;
	IXMLDOMElementPtr bda_template_connection, bdanode_descriptor, bda_node_properties, bda_node_types, bda_pin_types, bda_node_methods, bda_node_events;

	hr = CreateElement(pDoc,L"ksproperty_bda_template_connections",&bda_template_connection);
	hr = CreateElement(pDoc,L"ksproperty_bda_node_descriptors",&bdanode_descriptor);
	hr = CreateElement(pDoc,L"ksproperty_bda_node_properties",&bda_node_properties);
	hr = CreateElement(pDoc,L"ksproperty_bda_node_types",&bda_node_types);
	hr = CreateElement(pDoc,L"ksproperty_bda_pin_types",&bda_pin_types);
	hr = CreateElement(pDoc,L"ksproperty_bda_node_methods",&bda_node_methods);
	hr = CreateElement(pDoc,L"ksproperty_bda_node_events",&bda_node_events);

	for(size_t n = 0;n<t.bda_template_connection.size();n++)
	{
		hr = AddBDATemplateConnection(pDoc,bda_template_connection,t.bda_template_connection[n]);
	}

	for(size_t n = 0;n<t.bdanode_descriptor.size();n++)
	{
		hr = AddBDANodeDescriptor(pDoc,bdanode_descriptor,t.bdanode_descriptor[n]);
	}

	for(std::map<unsigned long,std::vector<GUID> >::const_iterator it=t.bda_node_properties.begin();it!=t.bda_node_properties.end();it++)
	{
		hr = AddGUIDList(pDoc,bda_node_properties,it);
	}

	for(size_t n = 0;n<t.bda_node_types.size();n++)
	{
		hr = AddULong(pDoc,bda_node_types,t.bda_node_types[n]);
	}

	for(size_t n = 0;n<t.bda_pin_types.size();n++)
	{
		hr = AddULong(pDoc,bda_pin_types,t.bda_pin_types[n]);
	}

	for(std::map<unsigned long,std::vector<GUID> >::const_iterator it=t.bda_node_methods.begin();it!=t.bda_node_methods.end();it++)
	{
		hr = AddGUIDList(pDoc,bda_node_methods,it);
	}

	for(std::map<unsigned long,std::vector<GUID> >::const_iterator it=t.bda_node_events.begin();it!=t.bda_node_events.end();it++)
	{
		hr = AddGUIDList(pDoc,bda_node_events,it);
	}

	hr = AppendChild(bda_template_connection,pParent);
	hr = AppendChild(bdanode_descriptor,pParent);
	hr = AppendChild(bda_node_properties,pParent);
	hr = AppendChild(bda_node_types,pParent);
	hr = AppendChild(bda_pin_types,pParent);
	hr = AppendChild(bda_node_methods,pParent);
	hr = AppendChild(bda_node_events,pParent);

	return S_OK;
}

static HRESULT AddBDATemplateConnection(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent, const BDA_TEMPLATE_CONNECTION& t)
{
	HRESULT hr;
	IXMLDOMElementPtr connection;

	hr = CreateElement(pDoc,L"connection",&connection);
	hr = AddAttribute(pDoc,L"FromNodeType",_variant_t(t.FromNodeType),connection);
	hr = AddAttribute(pDoc,L"FromNodePinType",_variant_t(t.FromNodePinType),connection);
	hr = AddAttribute(pDoc,L"ToNodeType",_variant_t(t.ToNodeType),connection);
	hr = AddAttribute(pDoc,L"ToNodePinType",_variant_t(t.ToNodePinType),connection);
	return AppendChild(connection,pParent);
}

static HRESULT AddTopologyConnection(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent, const KSTOPOLOGY_CONNECTION& t)
{
	HRESULT hr;
	IXMLDOMElementPtr connection;

	hr = CreateElement(pDoc,L"connection",&connection);
	hr = AddAttribute(pDoc,L"FromNode",_variant_t(t.FromNode),connection);
	hr = AddAttribute(pDoc,L"FromNodePin",_variant_t(t.FromNodePin),connection);
	hr = AddAttribute(pDoc,L"ToNode",_variant_t(t.ToNode),connection);
	hr = AddAttribute(pDoc,L"ToNodePin",_variant_t(t.ToNodePin),connection);
	return AppendChild(connection,pParent);
}

static HRESULT AddBDANodeDescriptor(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent, const BDANODE_DESCRIPTOR& t)
{
	HRESULT hr;
	IXMLDOMElementPtr descriptor;
	std::wstring guid;

	hr = CreateElement(pDoc,L"descriptor",&descriptor);
	hr = AddAttribute(pDoc,L"ulBdaNodeType",_variant_t(t.ulBdaNodeType),descriptor);
	hr = guid_to_string(t.guidFunction,guid);
	hr = AddAttribute(pDoc,L"guidFunction",guid.c_str(),descriptor);
	hr = guid_to_string(t.guidName,guid);
	hr = AddAttribute(pDoc,L"guidName",guid.c_str(),descriptor);

	return AppendChild(descriptor,pParent);
}

static HRESULT AddGUIDList(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,std::map<unsigned long,std::vector<GUID> >::const_iterator& l)
{
	HRESULT hr;
	IXMLDOMElementPtr node;

	hr = CreateElement(pDoc,L"node",&node);
	hr = AddAttribute(pDoc,L"id",_variant_t(l->first),node);

	for(size_t n=0;n<l->second.size();n++)
	{
		IXMLDOMElementPtr guid;
		std::wstring guidstr;

		hr = CreateElement(pDoc,L"guid",&guid);
		hr = guid_to_string(l->second[n],guidstr);
		hr = AddAttribute(pDoc,L"name",guidstr.c_str(),guid);
		hr = AppendChild(guid,node);
	}

	return AppendChild(node,pParent);
}

static HRESULT AddULong(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,unsigned long v)
{
	HRESULT hr;
	IXMLDOMElementPtr value;

	hr = CreateElement(pDoc,L"value",&value);
	hr = AddAttribute(pDoc,L"val",_variant_t(v),value);
	return AppendChild(value,pParent);
}

static HRESULT AddTopology(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const TOPOLOGY& t)
{
	HRESULT hr;
	IXMLDOMElementPtr categories, connection, name, nodes;

	hr = CreateElement(pDoc,L"ksproperty_topology_categories",&categories);
	hr = CreateElement(pDoc,L"ksproperty_topology_connections",&connection);
	hr = CreateElement(pDoc,L"ksproperty_topology_name",&name);
	hr = CreateElement(pDoc,L"ksproperty_topology_nodes",&nodes);

	for(size_t n=0;n<t.categories.size();n++)
	{
		hr = AddGUID(pDoc,categories,t.categories[n]);
	}

	for(size_t n=0;n<t.connection.size();n++)
	{
		hr = AddTopologyConnection(pDoc,connection,t.connection[n]);
	}

	for(size_t n=0;n<t.name.size();n++)
	{
		hr = AddString(pDoc,name,L"name",t.name[n].c_str());
	}

	for(size_t n=0;n<t.nodes.size();n++)
	{
		hr = AddGUID(pDoc,nodes,t.nodes[n]);
	}

	hr = AppendChild(categories,pParent);
	hr = AppendChild(connection,pParent);
	hr = AppendChild(name,pParent);
	hr = AppendChild(nodes,pParent);

	return S_OK;
}

static HRESULT AddGUID(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const GUID& g)
{
	HRESULT hr;
	IXMLDOMElementPtr guid;
	std::wstring guidstr;

	hr = CreateElement(pDoc,L"guid",&guid);
	hr = guid_to_string(g,guidstr);
	hr = AddAttribute(pDoc,L"name",guidstr.c_str(),guid);
	return AppendChild(guid,pParent);
}

static HRESULT AddString(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,LPCWSTR elementname,LPCWSTR value)
{
	HRESULT hr;
	IXMLDOMElementPtr name;

	hr = CreateElement(pDoc,elementname,&name);
	hr = AddAttribute(pDoc,L"value",value,name);
	return AppendChild(name,pParent);
}

static HRESULT AddPinTopology(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent, const PINTOPOLOGY& t)
{
	HRESULT hr;
	IXMLDOMElementPtr pins;

	hr = AddAttribute(pDoc,L"ksproperty_pin_ctypes",_variant_t(t.pin_ctypes),pParent);
	hr = CreateElement(pDoc,L"pins",&pins);

	for(size_t n=0;n<t.pininfo.size();n++)
	{
		hr = AddPinInfo(pDoc,pins,t.pininfo[n]);
	}

	return AppendChild(pins,pParent);
}

static HRESULT AddKSIDENTIFIER(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const KSIDENTIFIER& v);

static HRESULT AddPinInfo(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const PININFO& p)
{
	HRESULT hr;
	IXMLDOMElementPtr pin;
	hr = CreateElement(pDoc,L"pin",&pin);

	if(p.category_valid)
	{
		std::wstring guid;
		hr = guid_to_string(p.category,guid);
		hr = AddAttribute(pDoc,L"ksproperty_pin_category",guid.c_str(),pin);
	}

	if(p.cinstances_valid)
	{
		IXMLDOMElementPtr cinstances;
		hr = CreateElement(pDoc,L"ksproperty_pin_cinstances",&cinstances);
		hr = AddAttribute(pDoc,L"PossibleCount",_variant_t(p.cinstances.PossibleCount),cinstances);
		hr = AddAttribute(pDoc,L"CurrentCount",_variant_t(p.cinstances.CurrentCount),cinstances);
		hr = AppendChild(cinstances,pin);
	}

	if(p.communication_valid)
	{
		hr = AddAttribute(pDoc,L"ksproperty_pin_communication",_variant_t(p.communication),pin);
	}

	//if(p.constraineddataranges_valid)
	//{
	//}

	if(p.dataflow_valid)
	{
		hr = AddAttribute(pDoc,L"ksproperty_pin_dataflow",_variant_t(p.dataflow),pin);
	}

	if(p.dataintersection_valid)
	{
		hr = AddKSDATARANGE(pDoc,pin,L"ksproperty_pin_dataintersection",p.dataintersection);
	}

	IXMLDOMElementPtr dataranges;
	hr = CreateElement(pDoc,L"ksproperty_pin_dataranges",&dataranges);
	for(size_t n=0;n<p.dataranges.size();n++)
	{
		AddKSDATARANGE(pDoc,dataranges,L"ksdatarange",p.dataranges[n]);
	}
	hr = AppendChild(dataranges,pin);

	if(p.globalcinstances_valid)
	{
		IXMLDOMElementPtr globalcinstances;
		hr = CreateElement(pDoc,L"ksproperty_pin_globalcinstances",&globalcinstances);
		hr = AddAttribute(pDoc,L"PossibleCount",_variant_t(p.globalcinstances.PossibleCount),globalcinstances);
		hr = AddAttribute(pDoc,L"CurrentCount",_variant_t(p.globalcinstances.CurrentCount),globalcinstances);
		hr = AppendChild(globalcinstances,pin);
	}

	IXMLDOMElementPtr interfaces;
	hr = CreateElement(pDoc,L"ksproperty_pin_interfaces",&interfaces);
	for(size_t n=0;n<p.interfaces.size();n++)
	{
		hr = AddKSIDENTIFIER(pDoc,interfaces,p.interfaces[n]);
	}
	hr = AppendChild(interfaces,pin);

	IXMLDOMElementPtr mediums;
	hr = CreateElement(pDoc,L"ksproperty_pin_mediums",&mediums);
	for(size_t n=0;n<p.mediums.size();n++)
	{
		hr = AddKSIDENTIFIER(pDoc,interfaces,p.mediums[n]);
	}
	hr = AppendChild(mediums,pin);

	if(p.name_valid)
	{
		hr = AddAttribute(pDoc,L"ksproperty_pin_name",p.name.c_str(),pin);
	}

	if(p.necessaryinstances_valid)
	{
		hr = AddAttribute(pDoc,L"ksproperty_pin_necessaryinstances",_variant_t(p.necessaryinstances),pin);
	}

	if(p.physicalconnection_valid)
	{
		IXMLDOMElementPtr physicalconnection;
		hr = CreateElement(pDoc,L"ksproperty_pin_physicalconnection",&physicalconnection);
		hr = AddAttribute(pDoc,L"Size",_variant_t(p.physicalconnection.Size),physicalconnection);
		hr = AddAttribute(pDoc,L"Pin",_variant_t(p.physicalconnection.Pin),physicalconnection);
		hr = AddAttribute(pDoc,L"SymbolicLinkName",p.physicalconnection.SymbolicLinkName,physicalconnection);
		hr = AppendChild(physicalconnection,pin);
	}

	return AppendChild(pin,pParent);
}

static HRESULT AddKSDATARANGE(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,LPCWSTR name,const KSDATARANGE& v)
{
	HRESULT hr;
	IXMLDOMElementPtr element;
	std::wstring guid;
	hr = CreateElement(pDoc,name,&element);
	hr = AddAttribute(pDoc,L"FormatSize",_variant_t(v.FormatSize),element);
	hr = AddAttribute(pDoc,L"Flags",_variant_t(v.Flags),element);
	hr = AddAttribute(pDoc,L"SampleSize",_variant_t(v.SampleSize),element);
	hr = AddAttribute(pDoc,L"Reserved",_variant_t(v.Reserved),element);
	hr = guid_to_string(v.MajorFormat,guid);
	hr = AddAttribute(pDoc,L"MajorFormat",guid.c_str(),element);
	hr = guid_to_string(v.SubFormat,guid);
	hr = AddAttribute(pDoc,L"SubFormat",guid.c_str(),element);
	hr = guid_to_string(v.Specifier,guid);
	hr = AddAttribute(pDoc,L"Specifier",guid.c_str(),element);
	return AppendChild(element,pParent);
}

static HRESULT AddKSIDENTIFIER(IXMLDOMDocument* pDoc,IXMLDOMElement* pParent,const KSIDENTIFIER& v)
{
	HRESULT hr;
	IXMLDOMElementPtr element;
	std::wstring guid;

	hr = CreateElement(pDoc,L"ksidentifier",&element);
	hr = guid_to_string(v.Set,guid);
	hr = AddAttribute(pDoc,L"Set",guid.c_str(),element);
	hr = AddAttribute(pDoc,L"Id",_variant_t(v.Id),element);
	hr = AddAttribute(pDoc,L"Flags",_variant_t(v.Flags),element);
	return AppendChild(element,pParent);
}

static HRESULT AddAttribute(IXMLDOMDocument* pDoc,LPCWSTR name, LPCWSTR value,IXMLDOMElement* pParent)
{
	HRESULT hr;
	IXMLDOMAttributePtr Attribute, AttributeOut;

	hr = pDoc->createAttribute(_bstr_t(name),&Attribute);
	hr = Attribute->put_value(_variant_t(value));
	return pParent->setAttributeNode(Attribute,&AttributeOut);
}

static HRESULT AddAttribute(IXMLDOMDocument* pDoc,LPCWSTR name, _variant_t& value,IXMLDOMElement* pParent)
{
	HRESULT hr;
	IXMLDOMAttributePtr Attribute, AttributeOut;

	hr = pDoc->createAttribute(_bstr_t(name),&Attribute);
	hr = Attribute->put_value(value);
	return pParent->setAttributeNode(Attribute,&AttributeOut);
}

static HRESULT guid_to_string(REFCLSID rclsid,std::wstring& v)
{
	HRESULT hr;
	LPOLESTR lpolestr;
	if((hr=StringFromCLSID(rclsid,&lpolestr ))==S_OK)
	{
		v=lpolestr;
		CoTaskMemFree (lpolestr);
		return S_OK;
	}
	else
	{
		return hr;
	}
}

static HRESULT string_to_guid(LPCWSTR v,CLSID* rclsid)
{
	return CLSIDFromString(v,rclsid);
}

static HRESULT GetDeviceList(IXMLDOMElement* e, DEVICELIST& d)
{
	HRESULT hr;
	IXMLDOMNodeListPtr list;
	long listlength;

	hr = e->get_childNodes(&list);

	hr = list->get_length(&listlength);
	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMNodePtr itemnode;
		IXMLDOMElementPtr itemlement;

		hr = list->get_item(n,&itemnode);
		itemlement = itemnode;

		DEVICE device;
		hr = GetDevice(itemlement,device);
		d.push_back(device);

	}

	return S_OK;
}

static HRESULT GetDevice(IXMLDOMElement* e, DEVICE& d)
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
	d.device_path = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	hr = e->getAttribute(_bstr_t("DeviceDesc"),&v);
	d.DeviceDesc = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	hr = e->getAttribute(_bstr_t("Service"),&v);
	d.Service = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	hr = e->getAttribute(_bstr_t("Class"),&v);
	d.Class = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	hr = e->getAttribute(_bstr_t("ClassGUID"),&v);
	d.ClassGUID = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	hr = e->getAttribute(_bstr_t("Driver"),&v);
	d.Driver = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	hr = e->getAttribute(_bstr_t("PhysicalDeviceObjectName"),&v);
	d.PhysicalDeviceObjectName = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	hr = e->getAttribute(_bstr_t("Enumerator_Name"),&v);
	d.Enumerator_Name = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	hr = e->getAttribute(_bstr_t("device_instance_id"),&v);
	d.device_instance_id = (LPCWSTR)(_bstr_t)v;
	v.Clear();
	
	hr = e->selectSingleNode(_bstr_t(L"bdatopology"),&bdatopologynode);
	bdatopology = bdatopologynode;
	hr = e->selectSingleNode(_bstr_t(L"topology"),&topologynode);
	topology = topologynode;
	hr = e->selectSingleNode(_bstr_t(L"pintopology"),&pintopologynode);
	pintopology = pintopologynode;

	hr = GetBDATopology(bdatopology, d.bdatopology);
	hr = GetTopology(topology, d.topology);
	hr =  GetPinTopology(pintopology,d.pintopology);

	return S_OK;
}

static HRESULT GetBDATopology(IXMLDOMElement* e,BDATOPOLOGY& v)
{
	HRESULT hr;
	IXMLDOMElementPtr bda_template_connection, bdanode_descriptor, bda_node_properties, bda_node_types, bda_pin_types, bda_node_methods, bda_node_events;
	IXMLDOMNodePtr bda_template_connection_node, bdanode_descriptor_node, bda_node_properties_node, bda_node_types_node, bda_pin_types_node, bda_node_methods_node, bda_node_events_node;

	hr = e->selectSingleNode(_bstr_t(L"ksproperty_bda_template_connections"),&bda_template_connection_node);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_bda_node_descriptors"),&bdanode_descriptor_node);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_bda_node_properties"),&bda_node_properties_node);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_bda_node_types"),&bda_node_types_node);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_bda_pin_types"),&bda_pin_types_node);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_bda_node_methods"),&bda_node_methods_node);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_bda_node_events"),&bda_node_events_node);

	bda_template_connection=bda_template_connection_node;
	bdanode_descriptor=bdanode_descriptor_node;
	bda_node_properties=bda_node_properties_node;
	bda_node_types=bda_node_types_node;
	bda_pin_types=bda_pin_types_node;
	bda_node_methods=bda_node_methods_node;
	bda_node_events=bda_node_events_node;

	hr = GetBDATemplateConnections(bda_template_connection,v.bda_template_connection);
	hr = GetBDANodeDescriptors(bdanode_descriptor, v.bdanode_descriptor);
	hr = GetGUIDLists(bda_node_properties, v.bda_node_properties);
	hr = GetULongs(bda_node_types, v.bda_node_types);
	hr = GetULongs(bda_pin_types, v.bda_pin_types);
	hr = GetGUIDLists(bda_node_methods, v.bda_node_methods);
	hr = GetGUIDLists(bda_node_events, v.bda_node_events);

	return S_OK;
}

static HRESULT GetTopology(IXMLDOMElement* e,TOPOLOGY& v)
{
	HRESULT hr;
	IXMLDOMElementPtr categories, connection, name, nodes;
	IXMLDOMNodePtr  categories_node, connection_node, name_node, nodes_node;

	hr = e->selectSingleNode(_bstr_t(L"ksproperty_topology_categories"),&categories_node);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_topology_connections"),&connection_node);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_topology_name"),&name_node);
	hr = e->selectSingleNode(_bstr_t(L"ksproperty_topology_nodes"),&nodes_node);

	categories=categories_node;
	connection=connection_node;
	name=name_node;
	nodes=nodes_node;

	hr = GetGUIDs(categories,v.categories);
	hr = GetTopologyConnections(connection,v.connection);
	hr = GetStrings(name,v.name);
	hr = GetGUIDs(nodes,v.nodes);

	return S_OK;
}

static HRESULT GetPinTopology(IXMLDOMElement* e,PINTOPOLOGY& v)
{
	HRESULT hr;
	_variant_t var;
	IXMLDOMElementPtr pins;
	IXMLDOMNodePtr pins_node;

	hr = e->getAttribute(_bstr_t(L"ksproperty_pin_ctypes"),&var);
	v.pin_ctypes = (unsigned long)var;

	hr = e->selectSingleNode(_bstr_t(L""),&pins_node);
	pins = pins_node;

	hr = GetPininfos(pins,v.pininfo);

	return S_OK;
}

static HRESULT GetBDATemplateConnections(IXMLDOMElement* e,std::vector<BDA_TEMPLATE_CONNECTION>& v)
{
	HRESULT hr;
	IXMLDOMNodeListPtr children;
	long listlength;

	hr = e->get_childNodes(&children);
	children->get_length(&listlength);

	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMElementPtr item;
		IXMLDOMNodePtr itemnode;
		_variant_t var;
		BDA_TEMPLATE_CONNECTION con;

		hr = children->get_item(n,&itemnode);
		item = itemnode;

		hr = item->getAttribute(_bstr_t(L"FromNodeType"),&var);
		con.FromNodeType = (unsigned long)var;
		var.Clear();
		hr = item->getAttribute(_bstr_t(L"FromNodePinType"),&var);
		con.FromNodePinType = (unsigned long)var;
		var.Clear();
		hr = item->getAttribute(_bstr_t(L"ToNodeType"),&var);
		con.ToNodeType = (unsigned long)var;
		var.Clear();
		hr = item->getAttribute(_bstr_t(L"ToNodePinType"),&var);
		con.ToNodePinType = (unsigned long)var;
		var.Clear();
		v.push_back(con);
	}

	return S_OK;
}

static HRESULT GetBDANodeDescriptors(IXMLDOMElement* e,std::vector<BDANODE_DESCRIPTOR>& v)
{
	HRESULT hr;
	IXMLDOMNodeListPtr children;
	long listlength;

	hr = e->get_childNodes(&children);
	children->get_length(&listlength);

	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMElementPtr item;
		IXMLDOMNodePtr itemnode;
		_variant_t var;

		BDANODE_DESCRIPTOR con;

		hr = children->get_item(n,&itemnode);
		item = itemnode;

		hr = item->getAttribute(_bstr_t(L"ulBdaNodeType"),&var);
		con.ulBdaNodeType = (unsigned long)var;
		var.Clear();
		hr = item->getAttribute(_bstr_t(L"guidFunction"),&var);
		string_to_guid((LPCWSTR)(_bstr_t)var,&con.guidFunction);
		var.Clear();
		hr = item->getAttribute(_bstr_t(L"guidName"),&var);
		string_to_guid((LPCWSTR)(_bstr_t)var,&con.guidName);
		var.Clear();
	}
	return S_OK;
}

static HRESULT GetGUIDLists(IXMLDOMElement* e,std::map<unsigned long,std::vector<GUID> >& v)
{
	HRESULT hr;
	IXMLDOMNodeListPtr children;
	long listlength;

	hr = e->get_childNodes(&children);
	children->get_length(&listlength);

	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMElementPtr item;
		IXMLDOMNodePtr itemnode;
		_variant_t var;

		hr = children->get_item(n,&itemnode);
		item = itemnode;

		hr = item->getAttribute(_bstr_t(L"id"),&var);
		hr = GetGUIDs(item,v[(unsigned long)var]);

	}
	
	return S_OK;
}

static HRESULT GetULongs(IXMLDOMElement* e,std::vector<unsigned long>& v)
{
	HRESULT hr;
	IXMLDOMNodeListPtr children;
	long listlength;

	hr = e->get_childNodes(&children);
	children->get_length(&listlength);

	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMElementPtr item;
		IXMLDOMNodePtr itemnode;
		_variant_t var;
		unsigned long val;

		hr = children->get_item(n,&itemnode);
		item = itemnode;

		hr = item->getAttribute(_bstr_t(L"val"),&var);
		val = (unsigned long)var;
		v.push_back(val);
	}

	return S_OK;
}

static HRESULT GetGUIDs(IXMLDOMElement* e, std::vector<GUID>& v)
{
	HRESULT hr;
	IXMLDOMNodeListPtr children;
	long listlength;

	hr = e->get_childNodes(&children);
	children->get_length(&listlength);

	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMElementPtr item;
		IXMLDOMNodePtr itemnode;
		_variant_t var;
		GUID guid;

		hr = children->get_item(n,&itemnode);
		item = itemnode;

		hr = item->getAttribute(_bstr_t(L"name"),&var);
		hr = string_to_guid((LPCWSTR)(_bstr_t)var,&guid);
		v.push_back(guid);
	}

	return S_OK;
}

static HRESULT GetTopologyConnections(IXMLDOMElement* e, std::vector<KSTOPOLOGY_CONNECTION>& v)
{
	HRESULT hr;
	IXMLDOMNodeListPtr children;
	long listlength;

	hr = e->get_childNodes(&children);
	children->get_length(&listlength);

	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMElementPtr item;
		IXMLDOMNodePtr itemnode;
		_variant_t var;
		KSTOPOLOGY_CONNECTION con;

		hr = children->get_item(n,&itemnode);
		item = itemnode;

		hr = item->getAttribute(_bstr_t(L"FromNode"),&var);
		con.FromNode = (unsigned long)var;
		var.Clear();
		hr = item->getAttribute(_bstr_t(L"FromNodePin"),&var);
		con.FromNodePin = (unsigned long)var;
		var.Clear();
		hr = item->getAttribute(_bstr_t(L"ToNode"),&var);
		con.ToNode = (unsigned long)var;
		var.Clear();
		hr = item->getAttribute(_bstr_t(L"ToNodePin"),&var);
		con.ToNodePin = (unsigned long)var;
		var.Clear();
		v.push_back(con);
	}
	return S_OK;
}

static HRESULT GetStrings(IXMLDOMElement* e, std::vector<std::wstring>& v)
{
	HRESULT hr;
	IXMLDOMNodeListPtr children;
	long listlength;

	hr = e->get_childNodes(&children);
	children->get_length(&listlength);

	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMElementPtr item;
		IXMLDOMNodePtr itemnode;
		_variant_t var;
		std::wstring str;

		hr = children->get_item(n,&itemnode);
		item = itemnode;

		hr = item->getAttribute(_bstr_t(L"value"),&var);
		str = (LPCWSTR)(_bstr_t)var;
		v.push_back(str);
	}
	return S_OK;
}

static HRESULT GetPininfos(IXMLDOMElement* e,std::vector<PININFO>& v)
{
	HRESULT hr;
	IXMLDOMNodeListPtr children;
	long listlength;

	hr = e->get_childNodes(&children);
	children->get_length(&listlength);

	for(size_t n=0;n<listlength;n++)
	{
		IXMLDOMElementPtr item, item2;
		IXMLDOMNodePtr itemnode, itemnode2;
		_variant_t var;
		PININFO pininfo;

		hr = children->get_item(n,&itemnode);
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
			pininfo.cinstances.PossibleCount = (unsigned long)var;
			var.Clear();
			hr = item2->getAttribute(_bstr_t(L"CurrentCount"),&var);
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
			hr = GetKSDATARANGE(item2,pininfo.dataintersection);
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
			hr = children2->get_length(&listlength2);

			for(size_t m=0;m<listlength2;m++)
			{
				IXMLDOMElementPtr item3;
				IXMLDOMNodePtr itemnode3;
				KSDATARANGE ksdatarange;

				hr = children2->get_item(m, &itemnode3);
				item3 = itemnode3;

				hr = GetKSDATARANGE(item3,ksdatarange);
				pininfo.dataranges.push_back(ksdatarange);
			}

			pininfo.dataranges_valid = true;
			item2.Release();
			itemnode2.Release();
		}

		hr = item->selectSingleNode(_bstr_t(L"ksproperty_pin_globalcinstances"),&itemnode2);
		if(hr==S_OK)
		{
			item2 = itemnode2;
			hr = item2->getAttribute(_bstr_t(L"PossibleCount"),&var);
			pininfo.globalcinstances.PossibleCount = (unsigned long)var;
			var.Clear();
			hr = item2->getAttribute(_bstr_t(L"CurrentCount"),&var);
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
			hr = children2->get_length(&listlength2);

			for(size_t m=0;m<listlength2;m++)
			{
				IXMLDOMElementPtr item3;
				IXMLDOMNodePtr itemnode3;
				KSIDENTIFIER ksidentifier;

				hr = children2->get_item(m, &itemnode3);
				item3 = itemnode3;

				hr = GetKSIDENTIFIER(item3,ksidentifier);
				pininfo.interfaces.push_back(ksidentifier);
			}

			pininfo.interfaces_valid = true;
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
			hr = children2->get_length(&listlength2);

			for(size_t m=0;m<listlength2;m++)
			{
				IXMLDOMElementPtr item3;
				IXMLDOMNodePtr itemnode3;
				KSIDENTIFIER ksidentifier;

				hr = children2->get_item(m, &itemnode3);
				item3 = itemnode3;

				hr = GetKSIDENTIFIER(item3,ksidentifier);
				pininfo.mediums.push_back(ksidentifier);
			}

			pininfo.mediums_valid = true;
			itemnode2.Release();
			item2.Release();
		}

		hr = item->getAttribute(_bstr_t(L"ksproperty_pin_name"),&var);
		if(hr==S_OK)
		{
			pininfo.name = (LPCWSTR)(_bstr_t)var;
			pininfo.name_valid = true;
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
			pininfo.physicalconnection.Size = (unsigned long)var;
			var.Clear();
			hr = item2->getAttribute(_bstr_t(L"Pin"),&var);
			pininfo.physicalconnection.Pin = (unsigned long)var;
			var.Clear();
			hr = item2->getAttribute(_bstr_t(L"SymbolicLinkName"),&var);
			pininfo.symboliclinkname = (LPCWSTR)(_bstr_t)var;
			var.Clear();
			pininfo.physicalconnection_valid = true;
			itemnode2.Release();
			item2.Release();
		}

		v.push_back(pininfo);
	}

	return S_OK;
}

static HRESULT GetKSDATARANGE(IXMLDOMElement* e,KSDATARANGE& v)
{
	return S_OK;
}

static HRESULT GetKSIDENTIFIER(IXMLDOMElement* e,KSIDENTIFIER& v)
{
	return S_OK;
}
