#include <iostream>
#include <map>
#include "libwfp/filterengine.h"
#include "libwfp/objectenumerator.h"
#include <fstream>  
#include <codecvt>
#include <corecrt_io.h>
#include <fcntl.h>
#include <atlchecked.h>

std::ostream& operator<<(std::ostream& os, REFGUID guid) {

	os << std::uppercase;
	os.width(8);
	os << std::hex << guid.Data1 << '-';

	os.width(4);
	os << std::hex << guid.Data2 << '-';

	os.width(4);
	os << std::hex << guid.Data3 << '-';

	os.width(2);
	os << std::hex
		<< static_cast<short>(guid.Data4[0])
		<< static_cast<short>(guid.Data4[1])
		<< '-'
		<< static_cast<short>(guid.Data4[2])
		<< static_cast<short>(guid.Data4[3])
		<< static_cast<short>(guid.Data4[4])
		<< static_cast<short>(guid.Data4[5])
		<< static_cast<short>(guid.Data4[6])
		<< static_cast<short>(guid.Data4[7]);
	os << std::nouppercase;
	return os;
}

std::wostream& operator<<(std::wostream& os, REFGUID guid) {

	os << std::uppercase;
	os.width(8);
	os << std::hex << guid.Data1 << '-';

	os.width(4);
	os << std::hex << guid.Data2 << '-';

	os.width(4);
	os << std::hex << guid.Data3 << '-';

	os.width(2);
	os << std::hex
		<< static_cast<short>(guid.Data4[0])
		<< static_cast<short>(guid.Data4[1])
		<< '-'
		<< static_cast<short>(guid.Data4[2])
		<< static_cast<short>(guid.Data4[3])
		<< static_cast<short>(guid.Data4[4])
		<< static_cast<short>(guid.Data4[5])
		<< static_cast<short>(guid.Data4[6])
		<< static_cast<short>(guid.Data4[7]);
	os << std::nouppercase;
	return os;
}

std::wstring s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8_utf16<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8_utf16<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

GUID StringToGuid(const std::string& str)
{
	GUID guid;
	sscanf_s(str.c_str(),
		"{%8x-%4hx-%4hx-%2hhx%2hhx-%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx}",
		&guid.Data1, &guid.Data2, &guid.Data3,
		&guid.Data4[0], &guid.Data4[1], &guid.Data4[2], &guid.Data4[3],
		&guid.Data4[4], &guid.Data4[5], &guid.Data4[6], &guid.Data4[7]);

	return guid;
}

GUID StringToGuid(const std::wstring& wstr)
{
	return StringToGuid(ws2s(wstr));
}

std::wstring GuidToString(GUID guid)
{
	char guid_cstr[39];
	snprintf(guid_cstr, sizeof(guid_cstr),
		"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
	auto guid_str = std::string(guid_cstr);
	auto guid_wstr = s2ws(guid_str);
	return guid_wstr;
}

static const GUID ProviderKey = {
	0xa86fd1bf,
	0x21d0,
	0x6432,
	{ 0xa0, 0xbb, 0x17, 0x42, 0x5c, 0x88, 0x5c, 0x50 }
};

static const uint8_t ProviderData[]{
	0xaa, 0xbb, 0xcc, 0xdd, 0xee
};

typedef struct MY_FWPM_PROVIDER0_
{
	GUID providerKey;
	std::wstring name;
	std::wstring description;
	std::wstring serviceName;
} 	MY_FWPM_PROVIDER0_;


typedef struct MY_FWPM_SUBLAYER0_
{
	GUID subLayerKey;
	std::wstring name;
	std::wstring description;
	GUID providerKey;
	UINT16 weight;
} 	MY_FWPM_SUBLAYER0;

typedef struct MY_FWPM_FILTER0_
{
	GUID filterKey;
	std::wstring name;
	std::wstring description;
	GUID providerKey;
	GUID layerKey;
	GUID subLayerKey;
	FWP_VALUE0 weight;
	UINT32 numFilterConditions;
	/* [unique][size_is] */ FWPM_FILTER_CONDITION0* filterCondition;
	FWPM_ACTION0 action;
	/* [switch_is] */ /* [switch_type] */ union
	{
		/* [case()] */ UINT64 rawContext;
		/* [case()] */ GUID providerContextKey;
	};
	/* [unique] */ GUID* reserved;
	UINT64 filterId;
	FWP_VALUE0 effectiveWeight;
} 	MY_FWPM_FILTER0;

int main()
{
	std::wfstream fileContents;
	fileContents.open("test.yml", std::ios::out | std::ios::binary);
	
	auto engine = wfp::FilterEngine::DynamicSession();

	std::map<std::wstring, MY_FWPM_PROVIDER0_> providers;

	wfp::ObjectEnumerator::Providers(*engine, [&providers](const FWPM_PROVIDER0& provider)
		{
			auto myProvider = MY_FWPM_PROVIDER0_();
			if (provider.displayData.name) {
				myProvider.name = provider.displayData.name;
			}
			if (provider.displayData.description) {
				myProvider.description = provider.displayData.description;
			}
			myProvider.providerKey = provider.providerKey;
			if (provider.serviceName) {
				myProvider.serviceName = provider.serviceName;
			}
			providers.insert(std::make_pair(GuidToString(provider.providerKey), myProvider));
			return true;
		});

	std::map<std::wstring, FWPM_PROVIDER_CONTEXT0> providerContexts;
	std::multimap<std::wstring, FWPM_PROVIDER_CONTEXT0> providerContextsForProvider;
	wfp::ObjectEnumerator::ProviderContexts(*engine, [&](const FWPM_PROVIDER_CONTEXT0& providerContext)
		{
			providerContexts.insert(std::make_pair(GuidToString(providerContext.providerContextKey), providerContext));

			if (providerContext.providerKey != NULL) {
				providerContextsForProvider.insert(std::make_pair(GuidToString(*providerContext.providerKey), providerContext));
			}
			else {
				providerContextsForProvider.insert(std::make_pair(GuidToString(GUID_NULL), providerContext));
			}

			return true;
		});

	std::map<std::wstring, FWPM_LAYER0> layers;
	wfp::ObjectEnumerator::Layers(*engine, [&layers](const FWPM_LAYER0& layer)
		{
			layers.insert(std::make_pair(GuidToString(layer.layerKey), layer));
			return true;			
		});

	std::map<std::wstring, MY_FWPM_SUBLAYER0> sublayers;
	std::multimap<std::wstring, MY_FWPM_SUBLAYER0> sublayersForProvider;
	wfp::ObjectEnumerator::Sublayers(*engine, [&sublayers, &sublayersForProvider](const FWPM_SUBLAYER0& sublayer)
		{
			auto mySublayer = MY_FWPM_SUBLAYER0_();
			mySublayer.subLayerKey = sublayer.subLayerKey;
			if (sublayer.displayData.name) {
				mySublayer.name = sublayer.displayData.name;
			}
			if (sublayer.displayData.description) {
				mySublayer.description = sublayer.displayData.description;
			}
			if (sublayer.providerKey != NULL) {
				mySublayer.providerKey = *sublayer.providerKey;
			}
			else {
				mySublayer.providerKey = GUID_NULL;
			}

			sublayers.insert(std::make_pair(GuidToString(mySublayer.subLayerKey), mySublayer));
			sublayersForProvider.insert(std::make_pair(GuidToString(mySublayer.providerKey), mySublayer));
	
			return true;
		});

	std::map<std::wstring, MY_FWPM_FILTER0> filters;
	std::multimap<std::wstring, MY_FWPM_FILTER0> filtersForLayer;
	std::multimap<std::wstring, MY_FWPM_FILTER0> filtersForSubLayer;
	std::multimap<std::wstring, MY_FWPM_FILTER0> filtersForProvider;
	wfp::ObjectEnumerator::Filters(*engine, [&filters, &filtersForLayer, &filtersForSubLayer, &filtersForProvider](const FWPM_FILTER0& filter)
		{
			auto myFilter = MY_FWPM_FILTER0();
			myFilter.filterKey = filter.filterKey;
			myFilter.filterId = filter.filterId;
			if (filter.displayData.name) {
				myFilter.name = filter.displayData.name;
			}
			if (filter.displayData.description) {
				myFilter.description = filter.displayData.description;
			}
			if (filter.providerKey != NULL) {
				myFilter.providerKey = *filter.providerKey;
			}
			else {
				myFilter.providerKey = GUID_NULL;
			}
			myFilter.providerContextKey = filter.providerContextKey;
			myFilter.layerKey = filter.layerKey;
			myFilter.subLayerKey = filter.subLayerKey;
			myFilter.action = filter.action;
			myFilter.effectiveWeight = filter.effectiveWeight;
			myFilter.filterCondition = filter.filterCondition;
			myFilter.numFilterConditions = filter.numFilterConditions;
			myFilter.rawContext = filter.rawContext;
			myFilter.reserved = filter.reserved;
			myFilter.weight = filter.weight;

			filters.insert(std::make_pair(GuidToString(myFilter.filterKey), myFilter));
			filtersForLayer.insert(std::make_pair(GuidToString(myFilter.layerKey), myFilter));
			filtersForSubLayer.insert(std::make_pair(GuidToString(myFilter.subLayerKey), myFilter));
			filtersForProvider.insert(std::make_pair(GuidToString(myFilter.providerKey), myFilter));

			return true;
		});

	fileContents << std::endl << L"providers:" << std::endl;
	for (auto& currentProviderIterator : providers)
	{
		auto currentProvider = currentProviderIterator.second;
		auto currentSublayersForProvider = sublayersForProvider.equal_range(currentProviderIterator.first);

		fileContents << L"- providerKey: " << currentProvider.providerKey << std::endl;

		if (currentProvider.name != L"") {
			fileContents << L"  name: " << currentProvider.name << std::endl;
		}

		if (currentProvider.description != L"") {
			fileContents << L"  description: " << currentProvider.description << std::endl;
		}

		if (currentProvider.serviceName != L"") {
			fileContents << L"  serviceName: " << currentProvider.serviceName << std::endl;
		}

		if (currentSublayersForProvider.first != sublayersForProvider.end()) {
			fileContents << L"  subLayers:" << std::endl;
		}

		for (auto currentSublayersForProviderIterator = currentSublayersForProvider.first; currentSublayersForProviderIterator != sublayersForProvider.end(); ++currentSublayersForProviderIterator)
		{
			auto currentSublayer = currentSublayersForProviderIterator->second;
			auto currentfiltersForSubLayer = filtersForSubLayer.equal_range(currentSublayersForProviderIterator->first);
			
			auto subLayerTabbing = L"    ";
			fileContents << L"  - subLayerKey:" << currentSublayer.subLayerKey << std::endl;
			if (currentSublayer.name != L"") {
				fileContents << subLayerTabbing << L"name: " << currentSublayer.name << std::endl;
			}

			if (currentSublayer.description != L"") {
				fileContents << subLayerTabbing << L"description: " << currentSublayer.description << std::endl;
			}

			if (currentSublayer.weight > 0) {
				fileContents << subLayerTabbing << L"weight: " << currentSublayer.weight << std::endl;
			}

			if (currentfiltersForSubLayer.first != filtersForSubLayer.end()) {
				fileContents << subLayerTabbing << L"filters:" << std::endl;
			}

			for (auto currentfiltersForSubLayerIterator = currentfiltersForSubLayer.first; currentfiltersForSubLayerIterator != filtersForSubLayer.end(); ++currentfiltersForSubLayerIterator)
			{
				auto currentfilter = currentfiltersForSubLayerIterator->second;
				auto currentLayer = layers.at(GuidToString(currentfilter.layerKey));
				//auto currentProviderContext = providerContexts.at(GuidToString(currentfilter.providerContextKey));
				auto filterTabbing = L"        ";
				fileContents << L"      - filterKey: " << currentfilter.filterKey << std::endl;
				fileContents << filterTabbing << L"filterId: " << currentfilter.filterId << std::endl;
				if (currentfilter.name != L"") {
					fileContents << filterTabbing << L"name: " << currentfilter.name << std::endl;
				}

				if (currentfilter.description != L"") {
					fileContents << filterTabbing << L"description: " << currentfilter.description << std::endl;
				}

				switch (currentfilter.weight.type) {
				case FWP_DATA_TYPE_::FWP_INT8:
					fileContents << filterTabbing << L"weight: " << currentfilter.weight.int8 << std::endl;
					break;
				case FWP_DATA_TYPE_::FWP_INT16:
					fileContents << filterTabbing << L"weight: " << currentfilter.weight .int16<< std::endl;
					break;
				case FWP_DATA_TYPE_::FWP_INT32:
					fileContents << filterTabbing << L"weight: " << currentfilter.weight.int32 << std::endl;
					break;
				case FWP_DATA_TYPE_::FWP_UINT8:
					fileContents << filterTabbing << L"weight: " << currentfilter.weight.uint8 << std::endl;
					break;
				case FWP_DATA_TYPE_::FWP_UINT16:
					fileContents << filterTabbing << L"weight: " << currentfilter.weight.uint16 << std::endl;
					break;
				case FWP_DATA_TYPE_::FWP_UINT32:
					fileContents << filterTabbing << L"weight: " << currentfilter.weight.uint32 << std::endl;
					break;
				case FWP_DATA_TYPE_::FWP_FLOAT:
					fileContents << filterTabbing << L"weight: " << currentfilter.weight.float32 << std::endl;
					break;
				case FWP_DATA_TYPE_::FWP_DOUBLE:
					fileContents << filterTabbing << L"weight: " << currentfilter.weight.double64 << std::endl;
					break;
				}

				switch (currentfilter.effectiveWeight.type) {
				case FWP_DATA_TYPE_::FWP_INT8:
					fileContents << filterTabbing << L"effectiveWeight: " << currentfilter.effectiveWeight.int8 << std::endl;
					break;
				case FWP_DATA_TYPE_::FWP_INT16:
					fileContents << filterTabbing << L"effectiveWeight: " << currentfilter.effectiveWeight.int16 << std::endl;
					break;
				case FWP_DATA_TYPE_::FWP_INT32:
					fileContents << filterTabbing << L"effectiveWeight: " << currentfilter.effectiveWeight.int32 << std::endl;
					break;
				case FWP_DATA_TYPE_::FWP_UINT8:
					fileContents << filterTabbing << L"effectiveWeight: " << currentfilter.effectiveWeight.uint8 << std::endl;
					break;
				case FWP_DATA_TYPE_::FWP_UINT16:
					fileContents << filterTabbing << L"effectiveWeight: " << currentfilter.effectiveWeight.uint16 << std::endl;
					break;
				case FWP_DATA_TYPE_::FWP_UINT32:
					fileContents << filterTabbing << L"effectiveWeight: " << currentfilter.effectiveWeight.uint32 << std::endl;
					break;
				case FWP_DATA_TYPE_::FWP_FLOAT:
					fileContents << filterTabbing << L"effectiveWeight: " << currentfilter.effectiveWeight.float32 << std::endl;
					break;
				case FWP_DATA_TYPE_::FWP_DOUBLE:
					fileContents << filterTabbing << L"effectiveWeight: " << currentfilter.effectiveWeight.double64 << std::endl;
					break;
				}

			}
		}
	}

	fileContents.flush();
	fileContents.close();

	return 0;
}


