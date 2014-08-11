// MFCBDAINF - BDA DTV topology information application
// Copyright (C) 2014 Jens Vaaben <info@jensvaaben.com>, https://www.google.com/+JensVaaben http://www.dvbstreamexplorer.com

//#define _WIN32_WINNT 0x0501
#include <windows.h>	
#include <setupapi.h>
#include <stdio.h>
#include "bdainf.h"

//proprietary GUID
//TBS
static const GUID KSPROPSETID_BdaTunerExtensionProperties =
{0xfaa8f3e5, 0x31d4, 0x4e41, {0x88, 0xef, 0xd9, 0xeb, 0x71, 0x6f, 0x6e, 0xc9}};

DEFINE_GUIDSTRUCT( "C6EFE5EB-855A-4f1b-B7AA-87B5E1DC4113", KSPROPERTYSET_QBOXControl );
#define KSPROPERTYSET_QBOXControl DEFINE_GUIDNAMED( KSPROPERTYSET_QBOXControl )

const GUID KSPROPSETID_VIDCAP_CUSTOM_IRCAPTURE = 
{0xB51C4994,0x0054,0x4749,{0x82,0x43,0x02,0x9A,0x66,0x86,0x36,0x36}};

//Hauppauge
static const GUID KSPROPSETID_BdaTunerExtensionProperties_Hauppuage =
{0xfaa8f3e5, 0x31d4, 0x4e41, {0x88, 0xef, 0x00, 0xa0, 0xc9, 0xf2, 0x1f, 0xc7}};

//FireDTV
static const GUID KSPROPSETID_Firesat = { 0xab132414, 0xd060, 0x11d0, { 0x85, 0x83, 0x00, 0xc0, 0x4f, 0xd9, 0xba,0xf3 } };

//DVBWorld
#define STATIC_KSPROPSETID_DiSEqC12_3rd \
		0xb07f0a98,0xbab1,0x4a69,0x8a,0x18,0x29,0x36,0x84,0x9b,0x28,0xfc
DEFINE_GUIDSTRUCT("b07f0a98-bab1-4a69-8a18-2936849b28fc",KSPROPSETID_DiSEqC12_3rd);
#define KSPROPSETID_DiSEqC12_3rd DEFINE_GUIDNAMED(KSPROPSETID_DiSEqC12_3rd)

static const GUID GUID_DiseqcCmdSend = //DiseqcCmd interface GUID
	{ 0x61ae2cdf, 0x87e8, 0x445c, { 0x8a, 0x7, 0x35, 0x6e, 0xd2, 0x28, 0xfb, 0x4e } };

DEFINE_GUIDSTRUCT( "4C807F36-2DB7-44CE-9582-E1344782CB85", PCI_2002_TUNER_GUID );
#define PCI_2002_TUNER_GUID DEFINE_GUIDNAMED( PCI_2002_TUNER_GUID )

DEFINE_GUIDSTRUCT( "5A714CAD-60F9-4124-B922-8A0557B8840E", USB_2102_TUNER_GUID );
#define USB_2102_TUNER_GUID DEFINE_GUIDNAMED( USB_2102_TUNER_GUID )

DEFINE_GUIDSTRUCT( "F6694EB0-CD38-4775-98EC-56250C3FF950", PCI_4002_TUNER_GUID );
#define PCI_4002_TUNER_GUID DEFINE_GUIDNAMED( PCI_4002_TUNER_GUID )

void scan_bda_decives();
static void scan_bda_class(const GUID*,DEVICELIST& devicelist);
static void get_interface_detail(HDEVINFO hdevinfo,DWORD dwIdx,PSP_DEVICE_INTERFACE_DATA pData,DEVICE& device);
static void get_device_registry_info(HDEVINFO hdevinfo,DWORD dwIdx,PSP_DEVINFO_DATA psp_devinfo_data,DEVICE& device);
static void get_device_registry_info_property(HDEVINFO hdevinfo,DWORD dwIdx,PSP_DEVINFO_DATA psp_devinfo_data, LPCTSTR lpszPropName,DWORD Property,DEVICE& device);
static void process_device_path(DWORD dwIdx,LPCTSTR lpszPath,DEVICE& device);
static void get_bda_topology(DWORD dwIdx,HANDLE hFile,DEVICE& device);
static void get_topology(DWORD dwIdx,HANDLE hFile,DEVICE& device);
void guid_to_string(const GUID* guid,std::wstring& str);
static BOOL get_property_basic_support(DWORD dwIdx,HANDLE hFile,GUID propset,ULONG id,LPCTSTR strPropSet,LPCTSTR strPropId);
static void get_node_topology(DWORD dwIdx,HANDLE hFile,int node_id,DEVICE& device);
static void get_pin(DWORD dwIdx,HANDLE hFile,DEVICE& device); //KSPROPSETID_Pin

void dump_device_list(FILE* f,const BDADEVICES&);


void scan_bda_decives(BDADEVICES& bdadevices)
{
	bdadevices.reset();
	printf("enumerating: KSCATEGORY_BDA_NETWORK_TUNER\n");
	scan_bda_class(&KSCATEGORY_BDA_NETWORK_TUNER,bdadevices.bda_source);
	printf("enumerating: KSCATEGORY_BDA_RECEIVER_COMPONENT\n");
	scan_bda_class(&KSCATEGORY_BDA_RECEIVER_COMPONENT,bdadevices.bda_reciever);
}
static void scan_bda_class(const GUID* guid,DEVICELIST& devicelist)
{
	HDEVINFO hdevinfo=SetupDiGetClassDevs(guid,NULL,NULL,DIGCF_DEVICEINTERFACE|DIGCF_PRESENT);
	if(hdevinfo!=INVALID_HANDLE_VALUE)
	{
		// enum device interfaces
		SP_DEVICE_INTERFACE_DATA sp_device_interface_data;
		sp_device_interface_data.cbSize=sizeof(SP_DEVICE_INTERFACE_DATA);
		DWORD dwIdx=0;

		while(SetupDiEnumDeviceInterfaces(hdevinfo,NULL,guid,dwIdx,&sp_device_interface_data))
		{
			DEVICE device;
			get_interface_detail(hdevinfo,dwIdx,&sp_device_interface_data,device);
			devicelist.push_back(device);
			dwIdx++;
		}

		SetupDiDestroyDeviceInfoList(hdevinfo);
	}
	else
	{
		printf("failed SetupDiGetClassDevs() (%lu)\n",::GetLastError());
	}
}
static void get_interface_detail(HDEVINFO hdevinfo,DWORD dwIdx,PSP_DEVICE_INTERFACE_DATA pData,DEVICE& device)
{
	BYTE details[1024];
	((PSP_DEVICE_INTERFACE_DETAIL_DATA)&details[0])->cbSize=sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

	SP_DEVINFO_DATA sp_devinfo_data;
	sp_devinfo_data.cbSize=sizeof(SP_DEVINFO_DATA);

	if(SetupDiGetDeviceInterfaceDetail(hdevinfo,pData,(PSP_DEVICE_INTERFACE_DETAIL_DATA)details,sizeof(details)/sizeof(BYTE),NULL,&sp_devinfo_data))
	{
		device.device_path=((PSP_DEVICE_INTERFACE_DETAIL_DATA)&details[0])->DevicePath;
		printf("[%lu] device path: %s\n",dwIdx,((PSP_DEVICE_INTERFACE_DETAIL_DATA)&details[0])->DevicePath);
		get_device_registry_info(hdevinfo,dwIdx,&sp_devinfo_data,device);

		TCHAR szInstanceId[1024];
		if(SetupDiGetDeviceInstanceId(hdevinfo,&sp_devinfo_data,szInstanceId,sizeof(szInstanceId)/sizeof(TCHAR),NULL))
		{
			device.device_instance_id=szInstanceId;
			printf("[%lu] device instance id: %s\n",dwIdx,szInstanceId);
		}
		else
		{
			printf("[%lu] failed SetupDiGetDeviceInstanceId() (%lu)\n",dwIdx,::GetLastError());
		}
		process_device_path(dwIdx,((PSP_DEVICE_INTERFACE_DETAIL_DATA)&details[0])->DevicePath,device);
	}
	else
	{
		printf("[%lu] failed SetupDiGetDeviceInterfaceDetail() (%lu)\n",dwIdx,::GetLastError());
	}

}
static void get_device_registry_info(HDEVINFO hdevinfo,DWORD dwIdx,PSP_DEVINFO_DATA psp_devinfo_data,DEVICE& device)
{
	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"DeviceDesc",SPDRP_DEVICEDESC,device);
	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"HardwareID",SPDRP_HARDWAREID,device);
	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"CompatibleIDs",SPDRP_COMPATIBLEIDS,device);
	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"Service",SPDRP_SERVICE,device);

	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"Class",SPDRP_CLASS,device);
	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"ClassGUID",SPDRP_CLASSGUID,device);
	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"Driver",SPDRP_DRIVER,device);
	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"ConfigFlags",SPDRP_CONFIGFLAGS,device);
	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"Mfg",SPDRP_MFG,device);
	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"LocationInformation",SPDRP_LOCATION_INFORMATION,device);
	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"PhysicalDeviceObjectName",SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,device);
	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"Capabilities",SPDRP_CAPABILITIES,device);
	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"UiNumber",SPDRP_UI_NUMBER,device);
	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"BusTypeGUID",SPDRP_BUSTYPEGUID,device);
	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"LegacyBusType",SPDRP_LEGACYBUSTYPE,device);
	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"BusNumber",SPDRP_BUSNUMBER,device);
	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"Enumerator Name",SPDRP_ENUMERATOR_NAME,device);
	get_device_registry_info_property(hdevinfo,dwIdx,psp_devinfo_data,L"Device Address",SPDRP_ADDRESS,device);
}
static void get_device_registry_info_property(HDEVINFO hdevinfo,DWORD dwIdx,PSP_DEVINFO_DATA psp_devinfo_data, LPCTSTR lpszPropName,DWORD Property,DEVICE& device)
{
	BYTE buf[2048];
	DWORD dwType;
	DWORD dwRequiredSize;

	if(SetupDiGetDeviceRegistryProperty(hdevinfo,psp_devinfo_data,Property,&dwType,buf,sizeof(buf),&dwRequiredSize))
	{
		if(dwType==REG_DWORD)
		{
			printf("[%lu] %s: %lu\n",dwIdx,lpszPropName,*((DWORD*) &buf[0]));
		}
		else if(dwType==REG_SZ)
		{
			printf("[%lu] %s: %s\n",dwIdx,lpszPropName,((LPCTSTR*) &buf[0]));

			if(wcscmp(L"DeviceDesc",lpszPropName)==0)
			{
				device.DeviceDesc=(LPCWSTR)&buf[0];
			}
			else if(wcscmp(L"Service",lpszPropName)==0)
			{
				device.Service=(LPCWSTR)&buf[0];
			}

		}
		else
		{
			printf("[%lu] %s: property type %lu\n",dwIdx,lpszPropName,dwType);
		}
	}
	else
	{
		printf("[%lu] Failed SetupDiGetDeviceRegistryProperty(%s,%lu) (%lu)\n",dwIdx,lpszPropName,Property,::GetLastError());
	}
}
static void process_device_path(DWORD dwIdx,LPCTSTR lpszPath,DEVICE& device)
{
	HANDLE hFile;
	hFile=CreateFile(lpszPath,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
	if(hFile!=INVALID_HANDLE_VALUE)
	{
		get_bda_topology(dwIdx,hFile,device);
		get_topology(dwIdx,hFile,device);
		get_pin(dwIdx,hFile,device);
		CloseHandle(hFile);
	}
	else
	{
		printf("[%lu] failed to open device path (%lu)\n",dwIdx,::GetLastError());
	}
}
static void get_bda_topology(DWORD dwIdx,HANDLE hFile,DEVICE& device)
{
	DWORD dwBytesReturned;
	KSP_NODE ksp_node;
	BYTE output[1024];

	// first check if property set is supported

	if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_BdaTopology,KSPROPERTY_BDA_TEMPLATE_CONNECTIONS,L"KSPROPSETID_BdaTopology",L"KSPROPERTY_BDA_TEMPLATE_CONNECTIONS"))
	{
		ksp_node.Property.Set=KSPROPSETID_BdaTopology;
		ksp_node.Property.Id=KSPROPERTY_BDA_TEMPLATE_CONNECTIONS;
		ksp_node.Property.Flags=KSPROPERTY_TYPE_GET;

		//KSNODE_DESCRIPTOR ksnode_descriptor[100];

		if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_node,sizeof(KSP_NODE),output,sizeof(BDA_TEMPLATE_CONNECTION),&dwBytesReturned,NULL))
		{
			printf("[%lu] got IOCTL_KS_PROPERTY (bytes returned=%lu)\n",dwIdx,dwBytesReturned);

			int num_templates_connections = dwBytesReturned/sizeof(BDA_TEMPLATE_CONNECTION);
			BDA_TEMPLATE_CONNECTION* p = (BDA_TEMPLATE_CONNECTION*) &output[0];

			for(size_t n=0;n<num_templates_connections;n++)
			{
				device.bdatopology.bda_template_connection.push_back(p[n]);
				printf("[%d] [%d] FromNodeType=%lu\n",dwIdx,n,p[n].FromNodeType);
				printf("[%d] [%d] FromNodePinType=%lu\n",dwIdx,n,p[n].FromNodePinType);
				printf("[%d] [%d] ToNodeType=%lu\n",dwIdx,n,p[n].ToNodeType);
				printf("[%d] [%d] ToNodePinType=%lu\n",dwIdx,n,p[n].ToNodePinType);
			}
		}
		else
		{
			printf("[%lu] failed DeviceIoControl(IOCTL_KS_PROPERTY) (%lu)\n",dwIdx,::GetLastError());
		}
	}

	if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_BdaTopology,KSPROPERTY_BDA_NODE_DESCRIPTORS,L"KSPROPSETID_BdaTopology",L"KSPROPERTY_BDA_NODE_DESCRIPTORS"))
	{
		ksp_node.Property.Set=KSPROPSETID_BdaTopology;
		ksp_node.Property.Id=KSPROPERTY_BDA_NODE_DESCRIPTORS;
		ksp_node.Property.Flags=KSPROPERTY_TYPE_GET;

		if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_node,sizeof(KSPROPERTY),&output[0],sizeof(output)/sizeof(&output[0]),&dwBytesReturned,NULL))
		{
			printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_BdaTopology,KSPROPERTY_BDA_NODE_DESCRIPTORS) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);

			BDANODE_DESCRIPTOR* p = (BDANODE_DESCRIPTOR*) &output[0];
			unsigned node_descriptor=dwBytesReturned/sizeof(BDANODE_DESCRIPTOR);

			for(size_t n=0;n<node_descriptor;n++)
			{
				std::wstring strFunction, strName;

				guid_to_string(&p[n].guidFunction,strFunction);
				guid_to_string(&p[n].guidName,strName);

				printf("[%d] [%d] ulBdaNodeType=%lu\n",dwIdx,n,p[n].ulBdaNodeType);
				printf("[%d] [%d] guidFunction=%S\n",dwIdx,n,strFunction.c_str());
				printf("[%d] [%d] guidName =%S\n",dwIdx,n,strName.c_str());

				device.bdatopology.bdanode_descriptor.push_back(p[n]);

			}

		}
		else
		{
			printf("[%lu] failed DeviceIoControl(IOCTL_KS_PROPERTY) (%lu)\n",dwIdx,::GetLastError());
		}
	}


	if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_BdaTopology,KSPROPERTY_BDA_NODE_TYPES,L"KSPROPSETID_BdaTopology",L"KSPROPERTY_BDA_NODE_TYPES"))
	{
		ksp_node.Property.Set=KSPROPSETID_BdaTopology;
		ksp_node.Property.Id=KSPROPERTY_BDA_NODE_TYPES;
		ksp_node.Property.Flags=KSPROPERTY_TYPE_GET;

		if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_node,sizeof(KSPROPERTY),output,sizeof(output)/sizeof(&output[0]),&dwBytesReturned,NULL))
		{
			printf("[%lu] got IOCTL_KS_PROPERTY (bytes returned=%lu)\n",dwIdx,dwBytesReturned);


			int num_types=dwBytesReturned/sizeof(unsigned long);
			ULONG* types = (ULONG*) &output[0];

			for(size_t n=0;n<num_types;n++)
			{
				printf("[%lu] Node type [IDX=%u] %lu\n",dwIdx,n,types[n]);

				device.bdatopology.bda_node_types.push_back(types[n]);

				get_node_topology(dwIdx,hFile,types[n],device);
				

			}
		}
		else
		{
			printf("[%lu] failed DeviceIoControl(IOCTL_KS_PROPERTY) (%lu)\n",dwIdx,::GetLastError());
		}
	}

	if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_BdaTopology,KSPROPERTY_BDA_PIN_TYPES,L"KSPROPSETID_BdaTopology",L"KSPROPERTY_BDA_PIN_TYPES"))
	{
		ksp_node.Property.Set=KSPROPSETID_BdaTopology;
		ksp_node.Property.Id=KSPROPERTY_BDA_PIN_TYPES;
		ksp_node.Property.Flags=KSPROPERTY_TYPE_GET;

		if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_node,sizeof(KSPROPERTY),output,sizeof(output)/sizeof(&output[0]),&dwBytesReturned,NULL))
		{
			printf("[%lu] got IOCTL_KS_PROPERTY (bytes returned=%lu)\n",dwIdx,dwBytesReturned);

			unsigned pin_types=dwBytesReturned/sizeof(ULONG);
			ULONG* pins=(ULONG*)&output[0];

			for(size_t n=0;n<pin_types;n++)
			{
				device.bdatopology.bda_pin_types.push_back(pins[n]);
				printf("[%lu] [IDX=%d] %lu\n",dwIdx,n,pins[n]);
			}
		}
		else
		{
			printf("[%lu] failed DeviceIoControl(IOCTL_KS_PROPERTY) (%lu)\n",dwIdx,::GetLastError());
		}
	}

	if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_BdaTopology,KSPROPERTY_BDA_CONTROLLING_PIN_ID,L"KSPROPSETID_BdaTopology",L"KSPROPERTY_BDA_CONTROLLING_PIN_ID"))
	{
	}
}

static void get_topology(DWORD dwIdx,HANDLE hFile,DEVICE& device)
{
	DWORD dwBytesReturned;
	KSP_NODE ksp_node;
	BYTE output[1024];

	if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Topology,KSPROPERTY_TOPOLOGY_CATEGORIES,L"KSPROPSETID_Topology",L"KSPROPERTY_TOPOLOGY_CATEGORIES"))
	{

		ksp_node.Property.Set=KSPROPSETID_Topology;
		ksp_node.Property.Id=KSPROPERTY_TOPOLOGY_CATEGORIES;
		ksp_node.Property.Flags=KSPROPERTY_TYPE_GET;

		if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_node,sizeof(KSPROPERTY),&output[0],sizeof(output)/sizeof(&output[0]),&dwBytesReturned,NULL))
		{
			printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Topology,KSPROPERTY_TOPOLOGY_CATEGORIES) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);

			KSMULTIPLE_ITEM* pksmultiple_item = (KSMULTIPLE_ITEM*)&output[0];
			GUID* pkspin_interface= (GUID*)  &((KSMULTIPLE_ITEM*)output)[1];

			if( pksmultiple_item->Size==dwBytesReturned && (dwBytesReturned-sizeof(KSMULTIPLE_ITEM))/sizeof(GUID)==pksmultiple_item->Count && (dwBytesReturned-sizeof(KSMULTIPLE_ITEM))%sizeof(GUID)==0)
			{
				for(size_t n=0;n<pksmultiple_item->Count;n++)
				{
					device.topology.categories.push_back(pkspin_interface[n]);
				}
			}
			else
			{
				printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Topology,KSPROPERTY_TOPOLOGY_CATEGORIES) mismath size=%lu count=%lu\n",dwIdx,pksmultiple_item->Size,pksmultiple_item->Count);

			}
		}
		else
		{
			printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_TOPOLOGY_CATEGORIES) (%lu)\n",dwIdx,::GetLastError());
		}
	}

	if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Topology,KSPROPERTY_TOPOLOGY_NODES,L"KSPROPSETID_Topology",L"KSPROPERTY_TOPOLOGY_NODES"))
	{

		ksp_node.Property.Set=KSPROPSETID_Topology;
		ksp_node.Property.Id=KSPROPERTY_TOPOLOGY_NODES;
		ksp_node.Property.Flags=KSPROPERTY_TYPE_GET;

		if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_node,sizeof(KSPROPERTY),&output[0],sizeof(output)/sizeof(&output[0]),&dwBytesReturned,NULL))
		{
			printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Topology,KSPROPERTY_TOPOLOGY_NODES) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);

			KSMULTIPLE_ITEM* pksmultiple_item = (KSMULTIPLE_ITEM*)&output[0];
			GUID* pkspin_interface= (GUID*)  &((KSMULTIPLE_ITEM*)output)[1];

			if( pksmultiple_item->Size==dwBytesReturned && (dwBytesReturned-sizeof(KSMULTIPLE_ITEM))/sizeof(GUID)==pksmultiple_item->Count && (dwBytesReturned-sizeof(KSMULTIPLE_ITEM))%sizeof(GUID)==0)
			{
				for(size_t n=0;n<pksmultiple_item->Count;n++)
				{
					device.topology.nodes.push_back(pkspin_interface[n]);
				}
			}
			else
			{
				printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Topology,KSPROPERTY_TOPOLOGY_NODES) mismath size=%lu count=%lu\n",dwIdx,pksmultiple_item->Size,pksmultiple_item->Count);

			}
		}
		else
		{
			printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_TOPOLOGY_NODES) (%lu)\n",dwIdx,::GetLastError());
		}
	}


	if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Topology,KSPROPERTY_TOPOLOGY_CONNECTIONS,L"KSPROPSETID_Topology",L"KSPROPERTY_TOPOLOGY_CONNECTIONS"))
	{

		ksp_node.Property.Set=KSPROPSETID_Topology;
		ksp_node.Property.Id=KSPROPERTY_TOPOLOGY_CONNECTIONS;
		ksp_node.Property.Flags=KSPROPERTY_TYPE_GET;

		if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_node,sizeof(KSPROPERTY),&output,sizeof(output)/sizeof(&output[0]),&dwBytesReturned,NULL))
		{
			printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Topology,KSPROPERTY_TOPOLOGY_CONNECTIONS) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);

			KSMULTIPLE_ITEM* pksmultiple_item = (KSMULTIPLE_ITEM*)&output[0];
			KSTOPOLOGY_CONNECTION* pkspin_interface= (KSTOPOLOGY_CONNECTION*)  &((KSMULTIPLE_ITEM*)output)[1];

			if( pksmultiple_item->Size==dwBytesReturned && (dwBytesReturned-sizeof(KSMULTIPLE_ITEM))/sizeof(KSTOPOLOGY_CONNECTION)==pksmultiple_item->Count && (dwBytesReturned-sizeof(KSMULTIPLE_ITEM))%sizeof(KSTOPOLOGY_CONNECTION)==0)
			{
				for(size_t n=0;n<pksmultiple_item->Count;n++)
				{
					device.topology.connection.push_back(pkspin_interface[n]);
				}
			}
			else
			{
				printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Topology,KSPROPERTY_TOPOLOGY_CONNECTIONS) mismath size=%lu count=%lu\n",dwIdx,pksmultiple_item->Size,pksmultiple_item->Count);

			}
		}
		else
		{
			printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_TOPOLOGY_CONNECTIONS) (%lu)\n",dwIdx,::GetLastError());
		}
	}

	if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Topology,KSPROPERTY_TOPOLOGY_NAME,L"KSPROPSETID_Topology",L"KSPROPERTY_TOPOLOGY_NAME"))
	{

		ksp_node.Property.Set=KSPROPSETID_Topology;
		ksp_node.Property.Id=KSPROPERTY_TOPOLOGY_NAME;
		ksp_node.Property.Flags=KSPROPERTY_TYPE_GET;

		for(size_t n=0;n<device.topology.nodes.size();n++)
		{
			ksp_node.NodeId=n;

			if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_node,sizeof(KSP_NODE),&output,sizeof(output)/sizeof(&output[0]),&dwBytesReturned,NULL))
			{
				printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Topology,KSPROPERTY_TOPOLOGY_NAME) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);
				device.topology.name.push_back((LPCWSTR)&output[0]);

			}
			else
			{
				printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_TOPOLOGY_NAME) (%lu)\n",dwIdx,::GetLastError());
			}
		}
	}

}

static void get_pin(DWORD dwIdx,HANDLE hFile,DEVICE& device)
{
	DWORD dwBytesReturned;
	KSP_NODE ksp_node;
	BYTE output[1024];

	if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Pin,KSPROPERTY_TOPOLOGY_CATEGORIES,L"KSPROPSETID_Pin",L"KSPROPERTY_PIN_CTYPES"))
	{
		KSPROPERTY ksproperty;
		ksproperty.Set=KSPROPSETID_Pin;
		ksproperty.Id=KSPROPERTY_PIN_CTYPES;
		ksproperty.Flags=KSPROPERTY_TYPE_GET;

		if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksproperty,sizeof(KSPROPERTY),&device.pintopology.pin_ctypes,sizeof(ULONG),&dwBytesReturned,NULL))
		{
			printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Pin,KSPROPERTY_PIN_CTYPES) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);
			printf("[%lu] KSPROPERTY_PIN_CTYPES: %lu\n",dwIdx,device.pintopology.pin_ctypes);

			for(ULONG n=0;n<device.pintopology.pin_ctypes;n++)
			{
				
				KSP_PIN ksp_pin;

				ksp_pin.Property.Set=KSPROPSETID_Pin;
				ksp_pin.Property.Flags=KSPROPERTY_TYPE_GET;
				ksp_pin.PinId=n;
				ksp_pin.Reserved=0;

				PININFO pininfo;

				if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Pin,KSPROPERTY_PIN_CATEGORY,L"KSPROPSETID_Pin",L"KSPROPERTY_PIN_CATEGORY"))
				{
					ksp_pin.Property.Id=KSPROPERTY_PIN_CATEGORY;
					if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_pin,sizeof(KSP_PIN),&pininfo.category,sizeof(pininfo.category),&dwBytesReturned,NULL))
					{
						printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Pin,KSPROPERTY_PIN_CATEGORY) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);
						pininfo.category_valid=true;
					}
					else
					{
						printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_PIN_CATEGORY) (%lu)\n",dwIdx,::GetLastError());
					}
				}
				if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Pin,KSPROPERTY_PIN_CINSTANCES,L"KSPROPSETID_Pin",L"KSPROPERTY_PIN_CINSTANCES"))
				{
					ksp_pin.Property.Id=KSPROPERTY_PIN_CINSTANCES;
					if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_pin,sizeof(KSP_PIN),&pininfo.cinstances,sizeof(pininfo.cinstances),&dwBytesReturned,NULL))
					{
						printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Pin,KSPROPERTY_PIN_CINSTANCES) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);
						pininfo.cinstances_valid=true;
					}
					else
					{
						printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_PIN_CINSTANCES) (%lu)\n",dwIdx,::GetLastError());
					}
				}
				if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Pin,KSPROPERTY_PIN_COMMUNICATION,L"KSPROPSETID_Pin",L"KSPROPERTY_PIN_COMMUNICATION"))
				{
					ksp_pin.Property.Id=KSPROPERTY_PIN_COMMUNICATION;
					if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_pin,sizeof(KSP_PIN),&pininfo.communication,sizeof(pininfo.communication),&dwBytesReturned,NULL))
					{
						printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Pin,KSPROPERTY_PIN_COMMUNICATION) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);
						pininfo.communication_valid=true;
					}
					else
					{
						printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_PIN_COMMUNICATION) (%lu)\n",dwIdx,::GetLastError());
					}
				}
				if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Pin,KSPROPERTY_PIN_CONSTRAINEDDATARANGES,L"KSPROPSETID_Pin",L"KSPROPERTY_PIN_CONSTRAINEDDATARANGES"))
				{
					ksp_pin.Property.Id=KSPROPERTY_PIN_CONSTRAINEDDATARANGES;
					if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_pin,sizeof(KSP_PIN),&output,sizeof(output),&dwBytesReturned,NULL))
					{
						printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Pin,KSPROPERTY_PIN_CONSTRAINEDDATARANGES) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);
						pininfo.constraineddataranges_valid=true;
					}
					else
					{
						printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_PIN_CONSTRAINEDDATARANGES) (%lu)\n",dwIdx,::GetLastError());
					}
				}
				if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Pin,KSPROPERTY_PIN_DATAFLOW,L"KSPROPSETID_Pin",L"KSPROPERTY_PIN_DATAFLOW"))
				{
					ksp_pin.Property.Id=KSPROPERTY_PIN_DATAFLOW;
					if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_pin,sizeof(KSP_PIN),&pininfo.dataflow,sizeof(pininfo.dataflow),&dwBytesReturned,NULL))
					{
						printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Pin,KSPROPERTY_PIN_DATAFLOW) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);
						pininfo.dataflow_valid=true;
					}
					else
					{
						printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_PIN_DATAFLOW) (%lu)\n",dwIdx,::GetLastError());
					}
				}
				if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Pin,KSPROPERTY_PIN_DATAINTERSECTION,L"KSPROPSETID_Pin",L"KSPROPERTY_PIN_DATAINTERSECTION"))
				{
					ksp_pin.Property.Id=KSPROPERTY_PIN_DATAINTERSECTION;
					if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_pin,sizeof(KSP_PIN),&pininfo.dataintersection,sizeof(pininfo.dataintersection),&dwBytesReturned,NULL))
					{
						printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Pin,KSPROPERTY_PIN_DATAINTERSECTION) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);
						pininfo.dataintersection_valid=true;
					}
					else
					{
						printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_PIN_DATAINTERSECTION) (%lu)\n",dwIdx,::GetLastError());
					}
				}
				if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Pin,KSPROPERTY_PIN_DATARANGES,L"KSPROPSETID_Pin",L"KSPROPERTY_PIN_DATARANGES"))
				{
					ksp_pin.Property.Id=KSPROPERTY_PIN_DATARANGES;
					if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_pin,sizeof(KSP_PIN),&output[0],sizeof(output),&dwBytesReturned,NULL))
					{
						printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Pin,KSPROPERTY_PIN_DATARANGES) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);

						KSMULTIPLE_ITEM* pksmultiple_item = (KSMULTIPLE_ITEM*)&output[0];
						KSDATARANGE* pkspin_interface= (KSDATARANGE*)  &((KSMULTIPLE_ITEM*)output)[1];

						if( pksmultiple_item->Size==dwBytesReturned && (dwBytesReturned-sizeof(KSMULTIPLE_ITEM))/sizeof(KSDATARANGE)==pksmultiple_item->Count && (dwBytesReturned-sizeof(KSMULTIPLE_ITEM))%sizeof(KSDATARANGE)==0)
						{
							for(size_t n=0;n<pksmultiple_item->Count;n++)
							{
								pininfo.dataranges.push_back(pkspin_interface[n]);
							}
						}
						else
						{
							printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Pin,KSPROPERTY_PIN_DATARANGES) mismath size=%lu count=%lu\n",dwIdx,pksmultiple_item->Size,pksmultiple_item->Count);

						}
						pininfo.dataranges_valid=true;
					}
					else
					{
						printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_PIN_DATARANGES) (%lu)\n",dwIdx,::GetLastError());
					}
				}
				if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Pin,KSPROPERTY_PIN_GLOBALCINSTANCES,L"KSPROPSETID_Pin",L"KSPROPERTY_PIN_GLOBALCINSTANCES"))
				{
					ksp_pin.Property.Id=KSPROPERTY_PIN_GLOBALCINSTANCES;
					if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_pin,sizeof(KSP_PIN),&pininfo.globalcinstances,sizeof(pininfo.globalcinstances),&dwBytesReturned,NULL))
					{
						printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Pin,KSPROPERTY_PIN_GLOBALCINSTANCES) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);
						pininfo.globalcinstances_valid=true;
					}
					else
					{
						printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_PIN_GLOBALCINSTANCES) (%lu)\n",dwIdx,::GetLastError());
					}
				}
				if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Pin,KSPROPERTY_PIN_INTERFACES,L"KSPROPSETID_Pin",L"KSPROPERTY_PIN_INTERFACES"))
				{
					ksp_pin.Property.Id=KSPROPERTY_PIN_INTERFACES;
					if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_pin,sizeof(KSP_PIN),&output[0],sizeof(output),&dwBytesReturned,NULL))
					{
						printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Pin,KSPROPERTY_PIN_INTERFACES) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);

						KSMULTIPLE_ITEM* pksmultiple_item = (KSMULTIPLE_ITEM*)&output[0];
						KSPIN_INTERFACE* pkspin_interface= (KSPIN_INTERFACE*)  &((KSMULTIPLE_ITEM*)output)[1];

						if( pksmultiple_item->Size==dwBytesReturned && (dwBytesReturned-sizeof(KSMULTIPLE_ITEM))/sizeof(KSPIN_INTERFACE)==pksmultiple_item->Count && (dwBytesReturned-sizeof(KSMULTIPLE_ITEM))%sizeof(KSPIN_INTERFACE)==0)
						{
							for(size_t n=0;n<pksmultiple_item->Count;n++)
							{
								pininfo.interfaces.push_back(pkspin_interface[n]);
							}
						}
						else
						{
							printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Pin,KSPROPERTY_PIN_INTERFACES) mismath size=%lu count=%lu\n",dwIdx,pksmultiple_item->Size,pksmultiple_item->Count);

						}
						pininfo.interfaces_valid=true;
					}
					else
					{
						printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_PIN_INTERFACES) (%lu)\n",dwIdx,::GetLastError());
					}
				}
				if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Pin,KSPROPERTY_PIN_MEDIUMS,L"KSPROPSETID_Pin",L"KSPROPERTY_PIN_MEDIUMS"))
				{
					ksp_pin.Property.Id=KSPROPERTY_PIN_MEDIUMS;
					if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_pin,sizeof(KSP_PIN),&output[0],sizeof(output),&dwBytesReturned,NULL))
					{
						printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Pin,KSPROPERTY_PIN_MEDIUMS) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);
						KSMULTIPLE_ITEM* pksmultiple_item = (KSMULTIPLE_ITEM*)&output[0];
						KSPIN_MEDIUM* pkspin_interface= (KSPIN_MEDIUM*)  &((KSMULTIPLE_ITEM*)output)[1];

						if( pksmultiple_item->Size==dwBytesReturned && (dwBytesReturned-sizeof(KSMULTIPLE_ITEM))/sizeof(KSPIN_MEDIUM)==pksmultiple_item->Count && (dwBytesReturned-sizeof(KSMULTIPLE_ITEM))%sizeof(KSPIN_MEDIUM)==0)
						{
							for(size_t n=0;n<pksmultiple_item->Count;n++)
							{
								pininfo.mediums.push_back(pkspin_interface[n]);
							}
						}
						else
						{
							printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Pin,KSPROPERTY_PIN_MEDIUMS) mismath size=%lu count=%lu\n",dwIdx,pksmultiple_item->Size,pksmultiple_item->Count);

						}
						pininfo.mediums_valid=true;
					}
					else
					{
						printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_PIN_MEDIUMS) (%lu)\n",dwIdx,::GetLastError());
					}
				}
				if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Pin,KSPROPERTY_PIN_NAME,L"KSPROPSETID_Pin",L"KSPROPERTY_PIN_NAME"))
				{
					ksp_pin.Property.Id=KSPROPERTY_PIN_NAME;
					if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_pin,sizeof(KSP_PIN),&output[0],sizeof(output),&dwBytesReturned,NULL))
					{
						printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Pin,KSPROPERTY_PIN_NAME) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);
						pininfo.name=(LPCWSTR)&output[0];
						pininfo.name_valid=true;
					}
					else
					{
						printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_PIN_NAME) (%lu)\n",dwIdx,::GetLastError());
					}
				}
				if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Pin,KSPROPERTY_PIN_NECESSARYINSTANCES,L"KSPROPSETID_Pin",L"KSPROPERTY_PIN_NECESSARYINSTANCES"))
				{
					ksp_pin.Property.Id=KSPROPERTY_PIN_NECESSARYINSTANCES;
					if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_pin,sizeof(KSP_PIN),&pininfo.necessaryinstances,sizeof(pininfo.necessaryinstances),&dwBytesReturned,NULL))
					{
						printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Pin,KSPROPERTY_PIN_NECESSARYINSTANCES) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);
						pininfo.necessaryinstances_valid=true;
					}
					else
					{
						printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_PIN_NECESSARYINSTANCES) (%lu)\n",dwIdx,::GetLastError());
					}
				}
				if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_Pin,KSPROPERTY_PIN_PHYSICALCONNECTION,L"KSPROPSETID_Pin",L"KSPROPERTY_PIN_PHYSICALCONNECTION"))
				{
					ksp_pin.Property.Id=KSPROPERTY_PIN_PHYSICALCONNECTION;
					if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_pin,sizeof(KSP_PIN),&pininfo.physicalconnection,sizeof(pininfo.physicalconnection),&dwBytesReturned,NULL))
					{
						printf("[%lu] got IOCTL_KS_PROPERTY(KSPROPSETID_Pin,KSPROPERTY_PIN_PHYSICALCONNECTION) (bytes returned=%lu)\n",dwIdx,dwBytesReturned);
						pininfo.physicalconnection_valid=true;
					}
					else
					{
						printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_PIN_PHYSICALCONNECTION) (%lu)\n",dwIdx,::GetLastError());
					}
				}

				device.pintopology.pininfo.push_back(pininfo);

			}
		}
		else
		{
			printf("[%lu] failed DeviceIoControl(KSPROPSETID_Topology,KSPROPERTY_TOPOLOGY_CONNECTIONS) (%lu)\n",dwIdx,::GetLastError());
		}

	}
}

static void get_node_topology(DWORD dwIdx,HANDLE hFile,int node_id,DEVICE& device)
{
	DWORD dwBytesReturned;
	KSP_NODE ksp_node;
	BYTE output[1024];

	if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_BdaTopology,KSPROPERTY_BDA_NODE_PROPERTIES,L"KSPROPSETID_BdaTopology",L"KSPROPERTY_BDA_NODE_PROPERTIES"))
	{
		ksp_node.Property.Set=KSPROPSETID_BdaTopology;
		ksp_node.Property.Id=KSPROPERTY_BDA_NODE_PROPERTIES;
		ksp_node.Property.Flags=KSPROPERTY_TYPE_GET;
	
		ksp_node.NodeId=node_id;

		if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_node,sizeof(KSP_NODE/*KSPROPERTY*/),output,sizeof(output)/sizeof(&output[0]),&dwBytesReturned,NULL))
		{
			printf("[%lu] [%lu] got IOCTL_KS_PROPERTY (bytes returned=%lu)\n",dwIdx,node_id,dwBytesReturned);


			unsigned node_properties = dwBytesReturned/sizeof(GUID);

			GUID* p = (GUID*) &output[0];

			for(size_t n=0;n<node_properties;n++)
			{
				std::wstring strguid;
				guid_to_string(&p[n],strguid);
				printf("[%lu][%lu][%lu] %S\n",dwIdx,node_id,n,strguid.c_str());

				device.bdatopology.bda_node_properties[node_id].push_back(p[n]);

			}
		}
		else
		{
			printf("[%lu] failed DeviceIoControl(IOCTL_KS_PROPERTY) (%lu)\n",dwIdx,::GetLastError());
		}
	}

	if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_BdaTopology,KSPROPERTY_BDA_NODE_EVENTS,L"KSPROPSETID_BdaTopology",L"KSPROPERTY_BDA_NODE_EVENTS"))
	{
		ksp_node.Property.Set=KSPROPSETID_BdaTopology;
		ksp_node.Property.Id=KSPROPERTY_BDA_NODE_EVENTS;
		ksp_node.Property.Flags=KSPROPERTY_TYPE_GET;
	
		ksp_node.NodeId=node_id;

		if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_node,sizeof(KSP_NODE/*KSPROPERTY*/),output,sizeof(output)/sizeof(&output[0]),&dwBytesReturned,NULL))
		{
			printf("[%lu] [%lu] got IOCTL_KS_PROPERTY (bytes returned=%lu)\n",dwIdx,node_id,dwBytesReturned);


			unsigned node_properties = dwBytesReturned/sizeof(GUID);

			GUID* p = (GUID*) &output[0];

			for(size_t n=0;n<node_properties;n++)
			{
				std::wstring strguid;
				guid_to_string(&p[n],strguid);
				printf("[%lu][%lu][%lu] %S\n",dwIdx,node_id,n,strguid.c_str());

				device.bdatopology.bda_node_events[node_id].push_back(p[n]);

			}
		}
		else
		{
			printf("[%lu] failed DeviceIoControl(IOCTL_KS_PROPERTY) (%lu)\n",dwIdx,::GetLastError());
		}
	}

	if(get_property_basic_support(dwIdx,hFile,KSPROPSETID_BdaTopology,KSPROPERTY_BDA_NODE_METHODS,L"KSPROPSETID_BdaTopology",L"KSPROPERTY_BDA_NODE_METHODS"))
	{
		ksp_node.Property.Set=KSPROPSETID_BdaTopology;
		ksp_node.Property.Id=KSPROPERTY_BDA_NODE_METHODS;
		ksp_node.Property.Flags=KSPROPERTY_TYPE_GET;
	
		ksp_node.NodeId=node_id;

		if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_node,sizeof(KSP_NODE),output,sizeof(output)/sizeof(&output[0]),&dwBytesReturned,NULL))
		{
			printf("[%lu] [%lu] got IOCTL_KS_PROPERTY (bytes returned=%lu)\n",dwIdx,node_id,dwBytesReturned);


			unsigned node_properties = dwBytesReturned/sizeof(GUID);

			GUID* p = (GUID*) &output[0];

			for(size_t n=0;n<node_properties;n++)
			{
				std::wstring strguid;
				guid_to_string(&p[n],strguid);
				printf("[%lu][%lu][%lu] %S\n",dwIdx,node_id,n,strguid.c_str());

				device.bdatopology.bda_node_methods[node_id].push_back(p[n]);

			}
		}
		else
		{
			printf("[%lu] failed DeviceIoControl(IOCTL_KS_PROPERTY) (%lu)\n",dwIdx,::GetLastError());
		}
	}
}


static BOOL get_property_basic_support(DWORD dwIdx,HANDLE hFile,GUID propset,ULONG id,LPCTSTR strPropSet,LPCTSTR strPropId)
{
	DWORD dwBytesReturned;
	KSP_NODE ksp_node;
	BYTE output[1024];

	// first check if property set is supported
	ksp_node.Property.Set=propset;
	ksp_node.Property.Id=id;
	ksp_node.Property.Flags=KSPROPERTY_TYPE_BASICSUPPORT;

	if(DeviceIoControl(hFile,IOCTL_KS_PROPERTY,&ksp_node,sizeof(KSP_NODE),output,sizeof(KSPROPERTY_DESCRIPTION),&dwBytesReturned,NULL))
	{

		return TRUE;
	}
	else
	{
		return FALSE;
	}

}

void guid_to_string(const GUID* guid,std::wstring& str)
{

	if(IsEqualGUID(*guid,KSCATEGORY_BDA_RECEIVER_COMPONENT))
	{
		str=L"KSCATEGORY_BDA_RECEIVER_COMPONENT";
	}
	else if(IsEqualGUID(*guid,KSCATEGORY_BDA_NETWORK_TUNER))
	{
		str=L"KSCATEGORY_BDA_NETWORK_TUNER";
	}
	else if(IsEqualGUID(*guid,KSCATEGORY_BDA_NETWORK_EPG))
	{
		str=L"KSCATEGORY_BDA_NETWORK_EPG";
	}
	else if(IsEqualGUID(*guid,KSCATEGORY_BDA_NETWORK_PROVIDER))
	{
		str=L"KSCATEGORY_BDA_NETWORK_PROVIDER";
	}
	else if(IsEqualGUID(*guid,KSCATEGORY_BDA_TRANSPORT_INFORMATION))
	{
		str=L"KSCATEGORY_BDA_TRANSPORT_INFORMATION";
	}
	else if(IsEqualGUID(*guid,KSNODE_BDA_RF_TUNER))
	{
		str=L"KSNODE_BDA_RF_TUNER";
	}
	else if(IsEqualGUID(*guid,KSNODE_BDA_QAM_DEMODULATOR))
	{
		str=L"KSNODE_BDA_QAM_DEMODULATOR";
	}
	else if(IsEqualGUID(*guid,KSNODE_BDA_QPSK_DEMODULATOR))
	{
		str=L"KSNODE_BDA_QPSK_DEMODULATOR";
	}
	else if(IsEqualGUID(*guid,KSNODE_BDA_8VSB_DEMODULATOR))
	{
		str=L"KSNODE_BDA_8VSB_DEMODULATOR";
	}
	else if(IsEqualGUID(*guid,KSNODE_BDA_OPENCABLE_POD))
	{
		str=L"KSNODE_BDA_OPENCABLE_POD";
	}
	else if(IsEqualGUID(*guid,KSNODE_BDA_PID_FILTER))
	{
		str=L"KSNODE_BDA_PID_FILTER";
	}
	else if(IsEqualGUID(*guid,PINNAME_BDA_TRANSPORT))
	{
		str=L"PINNAME_BDA_TRANSPORT";
	}
	else if(IsEqualGUID(*guid,PINNAME_BDA_ANALOG_VIDEO))
	{
		str=L"PINNAME_BDA_ANALOG_VIDEO";
	}
	else if(IsEqualGUID(*guid,PINNAME_BDA_ANALOG_AUDIO))
	{
		str=L"PINNAME_BDA_ANALOG_AUDIO";
	}
	else if(IsEqualGUID(*guid,PINNAME_BDA_FM_RADIO))
	{
		str=L"PINNAME_BDA_FM_RADIO";
	}
	else if(IsEqualGUID(*guid,PINNAME_BDA_IF_PIN))
	{
		str=L"PINNAME_BDA_IF_PIN";
	}
	else if(IsEqualGUID(*guid,PINNAME_BDA_OPENCABLE_PSIP_PIN))
	{
		str=L"PINNAME_BDA_OPENCABLE_PSIP_PIN";
	}
	else if(IsEqualGUID(*guid,PINNAME_IPSINK_INPUT))
	{
		str=L"PINNAME_IPSINK_INPUT";
	}
	else if(IsEqualGUID(*guid,PINNAME_MPE))
	{
		str=L"PINNAME_MPE";
	}
	else if(IsEqualGUID(*guid,KSDATAFORMAT_TYPE_BDA_ANTENNA))
	{
		str=L"KSDATAFORMAT_TYPE_BDA_ANTENNA";
	}
	else if(IsEqualGUID(*guid,KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT))
	{
		str=L"KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT";
	}
	else if(IsEqualGUID(*guid,KSDATAFORMAT_SPECIFIER_BDA_TRANSPORT))
	{
		str=L"KSDATAFORMAT_SPECIFIER_BDA_TRANSPORT";
	}
	else if(IsEqualGUID(*guid,KSDATAFORMAT_TYPE_BDA_IF_SIGNAL))
	{
		str=L"KSDATAFORMAT_TYPE_BDA_IF_SIGNAL";
	}
	else if(IsEqualGUID(*guid,KSDATAFORMAT_TYPE_MPEG2_SECTIONS))
	{
		str=L"KSDATAFORMAT_TYPE_MPEG2_SECTIONS";
	}
	else if(IsEqualGUID(*guid,KSDATAFORMAT_SUBTYPE_ATSC_SI))
	{
		str=L"KSDATAFORMAT_SUBTYPE_ATSC_SI";
	}
	else if(IsEqualGUID(*guid,KSDATAFORMAT_SUBTYPE_DVB_SI ))
	{
		str=L"KSDATAFORMAT_SUBTYPE_DVB_SI ";
	}
	else if(IsEqualGUID(*guid,KSDATAFORMAT_SUBTYPE_BDA_OPENCABLE_PSIP))
	{
		str=L"KSDATAFORMAT_SUBTYPE_BDA_OPENCABLE_PSIP";
	}
	else if(IsEqualGUID(*guid,KSDATAFORMAT_SUBTYPE_BDA_OPENCABLE_OOB_PSIP))
	{
		str=L"KSDATAFORMAT_SUBTYPE_BDA_OPENCABLE_OOB_PSIP";
	}
	else if(IsEqualGUID(*guid,KSDATAFORMAT_TYPE_BDA_IP))
	{
		str=L"KSDATAFORMAT_TYPE_BDA_IP";
	}
	else if(IsEqualGUID(*guid,KSDATAFORMAT_SUBTYPE_BDA_IP))
	{
		str=L"KSDATAFORMAT_SUBTYPE_BDA_IP";
	}
	else if(IsEqualGUID(*guid,KSDATAFORMAT_SPECIFIER_BDA_IP))
	{
		str=L"KSDATAFORMAT_SPECIFIER_BDA_IP";
	}
	else if(IsEqualGUID(*guid,KSDATAFORMAT_TYPE_BDA_IP_CONTROL))
	{
		str=L"KSDATAFORMAT_TYPE_BDA_IP_CONTROL";
	}
	else if(IsEqualGUID(*guid,KSDATAFORMAT_SUBTYPE_BDA_IP_CONTROL))
	{
		str=L"KSDATAFORMAT_SUBTYPE_BDA_IP_CONTROL";
	}
	else if(IsEqualGUID(*guid,KSDATAFORMAT_TYPE_MPE))
	{
		str=L"KSDATAFORMAT_TYPE_MPE";
	}
	else if(IsEqualGUID(*guid,KSPROPSETID_BdaAutodemodulate))
	{
		str=L"KSPROPSETID_BdaAutodemodulate";
	}
	else if(IsEqualGUID(*guid,KSPROPSETID_BdaCA))
	{
		str=L"KSPROPSETID_BdaCA";
	}
	else if(IsEqualGUID(*guid,KSEVENTSETID_BdaCAEvent))
	{
		str=L"KSEVENTSETID_BdaCAEvent";
	}
	else if(IsEqualGUID(*guid,KSMETHODSETID_BdaChangeSync ))
	{
		str=L"KSMETHODSETID_BdaChangeSync";
	}
	else if(IsEqualGUID(*guid,KSMETHODSETID_BdaDeviceConfiguration))
	{
		str=L"KSMETHODSETID_BdaDeviceConfiguration";
	}
	else if(IsEqualGUID(*guid,KSPROPSETID_BdaDigitalDemodulator))
	{
		str=L"KSPROPSETID_BdaDigitalDemodulator";
	}
	else if(IsEqualGUID(*guid,KSPROPSETID_BdaFrequencyFilter))
	{
		str=L"KSPROPSETID_BdaFrequencyFilter";
	}
	else if(IsEqualGUID(*guid,KSPROPSETID_BdaLNBInfo))
	{
		str=L"KSPROPSETID_BdaLNBInfo";
	}
	else if(IsEqualGUID(*guid,KSPROPSETID_BdaNullTransform))
	{
		str=L"KSPROPSETID_BdaNullTransform";
	}
	else if(IsEqualGUID(*guid,KSPROPSETID_BdaPIDFilter))
	{
		str=L"KSPROPSETID_BdaPIDFilter";
	}
	else if(IsEqualGUID(*guid,KSPROPSETID_BdaPinControl))
	{
		str=L"KSPROPSETID_BdaPinControl";
	}
	else if(IsEqualGUID(*guid,KSEVENTSETID_BdaPinEvent))
	{
		str=L"KSEVENTSETID_BdaPinEvent";
	}
	else if(IsEqualGUID(*guid,KSPROPSETID_BdaSignalStats))
	{
		str=L"KSPROPSETID_BdaSignalStats";
	}
	else if(IsEqualGUID(*guid,KSPROPSETID_BdaTableSection))
	{
		str=L"KSPROPSETID_BdaTableSection";
	}
	else if(IsEqualGUID(*guid,KSPROPSETID_BdaTopology))
	{
		str=L"KSPROPSETID_BdaTopology";
	}
	else if(IsEqualGUID(*guid,KSPROPSETID_BdaVoidTransform))
	{
		str=L"KSPROPSETID_BdaVoidTransform";
	}
	else if(IsEqualGUID(*guid,KSNODE_BDA_COFDM_DEMODULATOR))
	{
		str=L"KSNODE_BDA_COFDM_DEMODULATOR";
	}
	else if(IsEqualGUID(*guid,KSINTERFACESETID_Standard))
	{
		str=L"KSINTERFACESETID_Standard";
	}
	else if(IsEqualGUID(*guid,KSINTERFACESETID_Media))
	{
		str=L"KSINTERFACESETID_Media";
	}
	else if(IsEqualGUID(*guid,KSMEDIUMSETID_Standard))
	{
		str=L"KSMEDIUMSETID_Standard";
	}
	else if(IsEqualGUID(*guid,KSDATAFORMAT_SUBTYPE_NONE))
	{
		str=L"KSDATAFORMAT_SUBTYPE_NONE";
	}
	else if(IsEqualGUID(*guid,KSDATAFORMAT_SPECIFIER_NONE))
	{
		str=L"KSDATAFORMAT_SPECIFIER_NONE";
	}
	else if(IsEqualGUID(*guid,KSPROPSETID_BdaTunerExtensionProperties))
	{
		str=L"KSPROPSETID_BdaTunerExtensionProperties (TBS)";
	}
	else if(IsEqualGUID(*guid,KSPROPERTYSET_QBOXControl))
	{
		str=L"KSPROPERTYSET_QBOXControl (TBS)";
	}
	else if(IsEqualGUID(*guid,KSPROPSETID_VIDCAP_CUSTOM_IRCAPTURE))
	{
		str=L"KSPROPSETID_VIDCAP_CUSTOM_IRCAPTURE (TBS)";
	}
	else if(IsEqualGUID(*guid,KSPROPSETID_BdaTunerExtensionProperties_Hauppuage))
	{
		str=L"KSPROPSETID_BdaTunerExtensionProperties (Hauppuage)";
	}
	else if(IsEqualGUID(*guid,KSPROPSETID_Firesat))
	{
		str=L"KSPROPSETID_Firesat (FireDTV)";
	}
	else if(IsEqualGUID(*guid,KSPROPSETID_DiSEqC12_3rd))
	{
		str=L"KSPROPSETID_DiSEqC12_3rd (DVBWorld)";
	}
	else if(IsEqualGUID(*guid,GUID_DiseqcCmdSend))
	{
		str=L"GUID_DiseqcCmdSend (DVBWorld)";
	}
	else if(IsEqualGUID(*guid,PCI_2002_TUNER_GUID))
	{
		str=L"PCI_2002_TUNER_GUID (DVBWorld)";
	}
	else if(IsEqualGUID(*guid,USB_2102_TUNER_GUID))
	{
		str=L"USB_2102_TUNER_GUID (DVBWorld)";
	}
	else if(IsEqualGUID(*guid,PCI_4002_TUNER_GUID))
	{
		str=L"PCI_4002_TUNER_GUID (DVBWorld)";
	}
	else
	{
		LPOLESTR lpolestr;
		if(StringFromCLSID(*guid,&lpolestr )==S_OK)
		{
			str=lpolestr;
			CoTaskMemFree (lpolestr);
		}
		else
		{
			str=L"failed to convert GUID to string";
		}
	}
}

void dump_device_list(FILE* f,const BDADEVICES& devices)
{
	fprintf(f,"KSCATEGORY_BDA_NETWORK_TUNER\n");

	for(size_t n=0;n<devices.bda_source.size();n++)
	{
		fprintf(f,"\t[%u] - %S\n",n,devices.bda_source[n].DeviceDesc.c_str());
		devices.bda_source[n].dump_device(f,2);
	}

	fprintf(f,"KSCATEGORY_BDA_RECEIVER_COMPONENT\n");

	for(size_t n=0;n<devices.bda_reciever.size();n++)
	{
		fprintf(f,"\t[%u] - %S\n",n,devices.bda_reciever[n].DeviceDesc.c_str());
		devices.bda_reciever[n].dump_device(f,2);
	}

}

void tagDEVICE::dump_device(FILE* f,int indent) const
{
	do_indent(f,indent);
	fprintf(f,"device path: %S\n",device_path.c_str());
	do_indent(f,indent);
	fprintf(f,"DeviceDesc: %S\n",DeviceDesc.c_str());

	bdatopology.dump_device(f,indent);
	topology.dump_device(f,indent);
	pintopology.dump_device(f,indent);
}

void tagDEVICE::do_indent(FILE* f,int indent)
{
	for(size_t n=0;n<indent;n++) fputc('\t',f);
}

void tagBDATOPOLOGY::dump_device(FILE* f,int indent) const
{
	tagDEVICE::do_indent(f,indent);
	fprintf(f,"KSPROPSETID_BdaTopology\n");
	tagDEVICE::do_indent(f,indent+1);
	fprintf(f,"KSPROPERTY_BDA_TEMPLATE_CONNECTIONS\n");

	for(size_t n=0;n<bda_template_connection.size();n++)
	{
		tagDEVICE::do_indent(f,indent+2);
		fprintf(f,"[%d] FromNodeType=%lu FromNodePinType=%lu ToNodeType=%lu ToNodePinType=%lu\n",n,
			bda_template_connection[n].FromNodeType,
			bda_template_connection[n].FromNodePinType,
			bda_template_connection[n].ToNodeType,
			bda_template_connection[n].ToNodePinType);
	}

	tagDEVICE::do_indent(f,indent+1);
	fprintf(f,"KSPROPERTY_BDA_NODE_PROPERTIES\n");
	
	for(std::map<unsigned long,std::vector<GUID> >::const_iterator it=bda_node_properties.begin();it!=bda_node_properties.end();it++)
	{
		tagDEVICE::do_indent(f,indent+2);
		fprintf(f,"node ID [%lu]\n",it->first);
		for(size_t n=0;n<it->second.size();n++)
		{
			tagDEVICE::do_indent(f,indent+3);
			std::wstring guid;
			guid_to_string(&(it->second[n]),guid);
			fprintf(f,"[%lu] %S\n",n,guid.c_str());
		}
	}

	tagDEVICE::do_indent(f,indent+1);
	fprintf(f,"KSPROPERTY_BDA_NODE_METHODS\n");
	
	for(std::map<unsigned long,std::vector<GUID> >::const_iterator it=bda_node_methods.begin();it!=bda_node_methods.end();it++)
	{
		tagDEVICE::do_indent(f,indent+2);
		fprintf(f,"node ID [%lu]\n",it->first);
		for(size_t n=0;n<it->second.size();n++)
		{
			tagDEVICE::do_indent(f,indent+3);
			std::wstring guid;
			guid_to_string(&(it->second[n]),guid);
			fprintf(f,"[%lu] %S\n",n,guid.c_str());
		}
	}

	tagDEVICE::do_indent(f,indent+1);
	fprintf(f,"KSPROPERTY_BDA_NODE_EVENTS\n");
	
	for(std::map<unsigned long,std::vector<GUID> >::const_iterator it=bda_node_events.begin();it!=bda_node_events.end();it++)
	{
		tagDEVICE::do_indent(f,indent+2);
		fprintf(f,"node ID [%lu]\n",it->first);
		for(size_t n=0;n<it->second.size();n++)
		{
			tagDEVICE::do_indent(f,indent+3);
			std::wstring guid;
			guid_to_string(&(it->second[n]),guid);
			fprintf(f,"[%lu] %S\n",n,guid.c_str());
		}
	}

	tagDEVICE::do_indent(f,indent+1);
	fprintf(f,"KSPROPERTY_BDA_NODE_DESCRIPTORS\n");

	for(size_t n=0;n<bdanode_descriptor.size();n++)
	{
		tagDEVICE::do_indent(f,indent+2);
		fprintf(f,"[%lu]\n",n);
		tagDEVICE::do_indent(f,indent+3);
		fprintf(f,"ulBdaNodeType [%lu]\n",bdanode_descriptor[n].ulBdaNodeType);

		std::wstring guid1, guid2;
		guid_to_string(&bdanode_descriptor[n].guidFunction,guid1);
		guid_to_string(&bdanode_descriptor[n].guidName,guid2);
		tagDEVICE::do_indent(f,indent+3);
		fprintf(f,"guidFunction %S\n",guid1.c_str());
		tagDEVICE::do_indent(f,indent+3);
		fprintf(f,"guidName %S\n",guid2.c_str());
	}


	tagDEVICE::do_indent(f,indent+1);
	fprintf(f,"KSPROPERTY_BDA_NODE_TYPES\n");
	for(size_t n=0;n<bda_node_types.size();n++)
	{
		tagDEVICE::do_indent(f,indent+2);
		fprintf(f,"[%d] %lu\n",n,bda_node_types[n]);
	}

	tagDEVICE::do_indent(f,indent+1);
	fprintf(f,"KSPROPERTY_BDA_PIN_TYPES\n");
	for(size_t n=0;n<bda_pin_types.size();n++)
	{
		tagDEVICE::do_indent(f,indent+2);
		fprintf(f,"[%d] %lu\n",n,bda_pin_types[n]);
	}

}

void tagPINTOPOLOGY::dump_device(FILE* f,int indent) const
{
	tagDEVICE::do_indent(f,indent);
	fprintf(f,"KSPROPSETID_Pin\n");
	tagDEVICE::do_indent(f,indent+1);
	fprintf(f,"KSPROPERTY_PIN_CTYPES [%lu]\n",pin_ctypes);

	for(ULONG n=0;n<pin_ctypes;n++)
	{
		tagDEVICE::do_indent(f,indent+2);
		fprintf(f,"pin [%lu]\n",n);
		pininfo[n].dump_device(f,indent+3);

	}
}

void tagPININFO::dump_device(FILE* f,int indent) const
{
	if(category_valid)
	{
		tagDEVICE::do_indent(f,indent);

		std::wstring strguid;
		guid_to_string(&category,strguid);

		fprintf(f,"KSPROPERTY_PIN_CATEGORY: %S\n",strguid.c_str());
	}
	else
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_CATEGORY not retrieved\n");
	}

	if(cinstances_valid)
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_CINSTANCES\n");
		tagDEVICE::do_indent(f,indent+1);
		fprintf(f,"PossibleCount=%lu\n",cinstances.PossibleCount);
		tagDEVICE::do_indent(f,indent+1);
		fprintf(f,"CurrentCount=%lu\n",cinstances.CurrentCount);
	}
	else
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_CINSTANCES not retrieved\n");
	}

	if(communication_valid)
	{
		std::string strtmp;
		if(communication==KSPIN_COMMUNICATION_NONE)
		{
			strtmp="KSPIN_COMMUNICATION_NONE";
		}
		else if(communication==KSPIN_COMMUNICATION_SINK)
		{
			strtmp="KSPIN_COMMUNICATION_SINK";
		}
		else if(communication==KSPIN_COMMUNICATION_SOURCE)
		{
			strtmp="KSPIN_COMMUNICATION_SOURCE";
		}
		else if(communication==KSPIN_COMMUNICATION_BOTH)
		{
			strtmp="KSPIN_COMMUNICATION_BOTH";
		}
		else if(communication==KSPIN_COMMUNICATION_BRIDGE)
		{
			strtmp="KSPIN_COMMUNICATION_BRIDGE";
		}
		else
		{
			char buf[64];
			_itoa_s(communication,buf,_countof(buf),10);
			strtmp=buf;
		}
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_COMMUNICATION: %s\n",strtmp.c_str());
	}
	else
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_COMMUNICATION not retrieved\n");
	}

	if(constraineddataranges_valid)
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_CONSTRAINEDDATARANGES\n");
	}
	else
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_CONSTRAINEDDATARANGES not retrieved\n");
	}

	if(dataflow_valid)
	{
		std::string strtmp;
		if(dataflow==KSPIN_DATAFLOW_IN)
		{
			strtmp="KSPIN_DATAFLOW_IN";
		}
		else if(dataflow==KSPIN_DATAFLOW_OUT)
		{
			strtmp="KSPIN_DATAFLOW_OUT";
		}
		else
		{
			char buf[64];
			_itoa_s(dataflow,buf,_countof(buf),10);
			strtmp=buf;
		}
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_DATAFLOW: %s\n",strtmp.c_str());
	}
	else
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_DATAFLOW not retrieved\n");
	}

	if(dataintersection_valid)
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_DATAINTERSECTION\n");
	}
	else
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_DATAINTERSECTION not retrieved\n");
	}

	if(dataranges_valid)
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_DATARANGES\n");
		for(size_t n=0;n<dataranges.size();n++)
		{
			tagDEVICE::do_indent(f,indent+1);
			fprintf(f,"[%u]\n",n);
			std::wstring MajorFormat, SubFormat, Specifier;

			guid_to_string(&dataranges[n].MajorFormat,MajorFormat);
			guid_to_string(&dataranges[n].SubFormat,SubFormat);
			guid_to_string(&dataranges[n].Specifier,Specifier);

			tagDEVICE::do_indent(f,indent+2);
			fprintf(f,"FormatSize: %lu\n",dataranges[n].FormatSize);
			tagDEVICE::do_indent(f,indent+2);
			fprintf(f,"Flags: %lu\n",dataranges[n].Flags);
			tagDEVICE::do_indent(f,indent+2);
			fprintf(f,"SampleSize: %lu\n",dataranges[n].SampleSize);
			tagDEVICE::do_indent(f,indent+2);
			fprintf(f,"Reserved: %lu\n",dataranges[n].Reserved);
			tagDEVICE::do_indent(f,indent+2);
			fprintf(f,"MajorFormat: %S\n",MajorFormat.c_str());
			tagDEVICE::do_indent(f,indent+2);
			fprintf(f,"SubFormat: %S\n",SubFormat.c_str());
			tagDEVICE::do_indent(f,indent+2);
			fprintf(f,"Specifier: %S\n",Specifier.c_str());
		}
	}
	else
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_DATARANGES not retrieved\n");
	}

	if(globalcinstances_valid)
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_GLOBALCINSTANCES\n");
		tagDEVICE::do_indent(f,indent+1);
		fprintf(f,"PossibleCount=%lu\n",globalcinstances.PossibleCount);
		tagDEVICE::do_indent(f,indent+1);
		fprintf(f,"CurrentCount=%lu\n",globalcinstances.CurrentCount);
	}
	else
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_GLOBALCINSTANCES not retrieved\n");
	}

	if(interfaces_valid)
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_INTERFACES\n");
		for(size_t n=0;n<interfaces.size();n++)
		{
			tagDEVICE::do_indent(f,indent+1);
			fprintf(f,"[%lu]\n",n);

			std::wstring strguid;
			guid_to_string(&interfaces[n].Set ,strguid);
			tagDEVICE::do_indent(f,indent+2);
			fprintf(f,"Set: %S\n",strguid.c_str());
			tagDEVICE::do_indent(f,indent+2);
			fprintf(f,"Id: %lu\n",interfaces[n].Id);
			tagDEVICE::do_indent(f,indent+2);
			fprintf(f,"Flags: 0x%04x\n",interfaces[n].Flags);
		}
	}
	else
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_INTERFACES not retrieved\n");
	}

	if(mediums_valid)
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_MEDIUMS\n");
		for(size_t n=0;n<mediums.size();n++)
		{
			tagDEVICE::do_indent(f,indent+1);
			fprintf(f,"[%lu]\n",n);

			std::wstring strguid;
			guid_to_string(&mediums[n].Set ,strguid);
			tagDEVICE::do_indent(f,indent+2);
			fprintf(f,"Set: %S\n",strguid.c_str());
			tagDEVICE::do_indent(f,indent+2);
			fprintf(f,"Id: %lu\n",mediums[n].Id);
			tagDEVICE::do_indent(f,indent+2);
			fprintf(f,"Flags: 0x%04x\n",mediums[n].Flags);
		}
	}
	else
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_MEDIUMS not retrieved\n");
	}

	if(name_valid)
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_NAME: %S\n",name.c_str());
	}
	else
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_NAME not retrieved\n");
	}

	if(necessaryinstances_valid)
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_NECESSARYINSTANCES: %lu\n",necessaryinstances);
	}
	else
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_NECESSARYINSTANCES not retrieved\n");
	}

	if(physicalconnection_valid)
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_PHYSICALCONNECTION\n");
	}
	else
	{
		tagDEVICE::do_indent(f,indent);
		fprintf(f,"KSPROPERTY_PIN_PHYSICALCONNECTION not retrieved\n");
	}
}

void tagTOPOLOGY::dump_device(FILE* f,int indent) const
{
	tagDEVICE::do_indent(f,indent);
	fprintf(f,"KSPROPSETID_Topology\n");

	tagDEVICE::do_indent(f,indent+1);
	fprintf(f,"KSPROPERTY_TOPOLOGY_CATEGORIES\n");
	for(size_t n=0;n<categories.size();n++)
	{
		std::wstring strguid;
		guid_to_string(&categories[n],strguid);
		tagDEVICE::do_indent(f,indent+2);
		fprintf(f,"[%u] %S\n",n,strguid.c_str());
	}

	tagDEVICE::do_indent(f,indent+1);
	fprintf(f,"KSPROPERTY_TOPOLOGY_CONNECTIONS\n");
	for(size_t n=0;n<connection.size();n++)
	{
		tagDEVICE::do_indent(f,indent+2);
		fprintf(f,"[%u]\n",n);

		tagDEVICE::do_indent(f,indent+3);
		fprintf(f,"FromNode=%lu\n",connection[n].FromNode);
		tagDEVICE::do_indent(f,indent+3);
		fprintf(f,"FromNodePin=%lu\n",connection[n].FromNodePin);
		tagDEVICE::do_indent(f,indent+3);
		fprintf(f,"ToNode=%lu\n",connection[n].ToNode);
		tagDEVICE::do_indent(f,indent+3);
		fprintf(f,"ToNodePin=%lu\n",connection[n].ToNodePin);

	}



	tagDEVICE::do_indent(f,indent+1);
	fprintf(f,"KSPROPERTY_TOPOLOGY_NAME\n");
	for(size_t n=0;n<name.size();n++)
	{
		tagDEVICE::do_indent(f,indent+2);
		fprintf(f,"[%u] %S\n",n,name[n].c_str());
	}


	tagDEVICE::do_indent(f,indent+1);
	fprintf(f,"KSPROPERTY_TOPOLOGY_NODES\n");
	for(size_t n=0;n<nodes.size();n++)
	{
		std::wstring strguid;
		guid_to_string(&nodes[n],strguid);
		tagDEVICE::do_indent(f,indent+2);
		fprintf(f,"[%u] %S\n",n,strguid.c_str());
	}
}
