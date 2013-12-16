
#ifndef WIN32_NO_STATUS
#include <ntstatus.h>
#define WIN32_NO_STATUS
#endif
#include <unknwn.h>
#include "RdpCredential.h"
#include "guid.h"

RdpCredential::RdpCredential():
	_cRef(1),
	_pCredProvCredentialEvents(NULL)
{
	DllAddRef();

	ZeroMemory(_rgCredProvFieldDescriptors, sizeof(_rgCredProvFieldDescriptors));
	ZeroMemory(_rgFieldStatePairs, sizeof(_rgFieldStatePairs));
	ZeroMemory(_rgFieldStrings, sizeof(_rgFieldStrings));
}

RdpCredential::~RdpCredential()
{
	if (_rgFieldStrings[SFI_PASSWORD])
	{
		size_t lenPassword;
		HRESULT hr = StringCchLengthW(_rgFieldStrings[SFI_PASSWORD], 128, &(lenPassword));

		if (SUCCEEDED(hr))
		{
			SecureZeroMemory(_rgFieldStrings[SFI_PASSWORD], lenPassword * sizeof(*_rgFieldStrings[SFI_PASSWORD]));
		}
		else
		{

		}
	}

	for (int i = 0; i < ARRAYSIZE(_rgFieldStrings); i++)
	{
		CoTaskMemFree(_rgFieldStrings[i]);
		CoTaskMemFree(_rgCredProvFieldDescriptors[i].pszLabel);
	}

	DllRelease();
}

HRESULT RdpCredential::Initialize(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
	const CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR* rgcpfd,
	const FIELD_STATE_PAIR* rgfsp, PCWSTR pwzUsername, PCWSTR pwzPassword)
{
	HRESULT hr = S_OK;

	_cpus = cpus;

	for (DWORD i = 0; SUCCEEDED(hr) && i < ARRAYSIZE(_rgCredProvFieldDescriptors); i++)
	{
		_rgFieldStatePairs[i] = rgfsp[i];
		hr = FieldDescriptorCopy(rgcpfd[i], &_rgCredProvFieldDescriptors[i]);
	}
	
	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(pwzUsername, &_rgFieldStrings[SFI_USERNAME]);
	}
	
	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(pwzPassword ? pwzPassword : L"", &_rgFieldStrings[SFI_PASSWORD]);
	}

	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(L"Submit", &_rgFieldStrings[SFI_SUBMIT_BUTTON]);
	}

	return S_OK;
}

HRESULT RdpCredential::Advise(ICredentialProviderCredentialEvents* pcpce)
{
	if (_pCredProvCredentialEvents != NULL)
	{
		_pCredProvCredentialEvents->Release();
	}

	_pCredProvCredentialEvents = pcpce;
	_pCredProvCredentialEvents->AddRef();

	return S_OK;
}

HRESULT RdpCredential::UnAdvise()
{
	if (_pCredProvCredentialEvents)
	{
		_pCredProvCredentialEvents->Release();
	}

	_pCredProvCredentialEvents = NULL;

	return S_OK;
}

HRESULT RdpCredential::SetSelected(BOOL* pbAutoLogon)  
{
	*pbAutoLogon = FALSE;  

	return S_OK;
}

HRESULT RdpCredential::SetDeselected()
{
	HRESULT hr = S_OK;
	
	if (_rgFieldStrings[SFI_PASSWORD])
	{
		size_t lenPassword;
		hr = StringCchLengthW(_rgFieldStrings[SFI_PASSWORD], 128, &(lenPassword));
		if (SUCCEEDED(hr))
		{
			SecureZeroMemory(_rgFieldStrings[SFI_PASSWORD], lenPassword * sizeof(*_rgFieldStrings[SFI_PASSWORD]));

			CoTaskMemFree(_rgFieldStrings[SFI_PASSWORD]);
			hr = SHStrDupW(L"", &_rgFieldStrings[SFI_PASSWORD]);
		}

		if (SUCCEEDED(hr) && _pCredProvCredentialEvents)
		{
			_pCredProvCredentialEvents->SetFieldString(this, SFI_PASSWORD, _rgFieldStrings[SFI_PASSWORD]);
		}
	}

	return hr;
}

HRESULT RdpCredential::GetFieldState(DWORD dwFieldID, CREDENTIAL_PROVIDER_FIELD_STATE* pcpfs,
	CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE* pcpfis)
{
	HRESULT hr;

	if ((dwFieldID < ARRAYSIZE(_rgFieldStatePairs)) && pcpfs && pcpfis)
	{
		*pcpfs = _rgFieldStatePairs[dwFieldID].cpfs;
		*pcpfis = _rgFieldStatePairs[dwFieldID].cpfis;

		hr = S_OK;
	}
	else
	{
		hr = E_INVALIDARG;
	}
	return hr;
}

HRESULT RdpCredential::GetStringValue(DWORD dwFieldID, PWSTR* ppwsz)
{
	HRESULT hr;

	if (dwFieldID < ARRAYSIZE(_rgCredProvFieldDescriptors) && ppwsz) 
	{
		hr = SHStrDupW(_rgFieldStrings[dwFieldID], ppwsz);
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

HRESULT RdpCredential::GetBitmapValue(DWORD dwFieldID, HBITMAP* phbmp)
{
	HRESULT hr;

	if ((SFI_TILEIMAGE == dwFieldID) && phbmp)
	{
		HBITMAP hbmp = LoadBitmap(HINST_THISDLL, MAKEINTRESOURCE(IDB_TILE_IMAGE));

		if (hbmp != NULL)
		{
			hr = S_OK;
			*phbmp = hbmp;
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

HRESULT RdpCredential::GetSubmitButtonValue(DWORD dwFieldID, DWORD* pdwAdjacentTo)
{
	HRESULT hr;

	if ((SFI_SUBMIT_BUTTON == dwFieldID) && pdwAdjacentTo)
	{
		*pdwAdjacentTo = SFI_PASSWORD;
		hr = S_OK;
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

HRESULT RdpCredential::SetStringValue(DWORD dwFieldID, PCWSTR pwz)
{
	HRESULT hr;

	if (dwFieldID < ARRAYSIZE(_rgCredProvFieldDescriptors) && 
		(CPFT_EDIT_TEXT == _rgCredProvFieldDescriptors[dwFieldID].cpft || 
		CPFT_PASSWORD_TEXT == _rgCredProvFieldDescriptors[dwFieldID].cpft)) 
	{
		PWSTR* ppwszStored = &_rgFieldStrings[dwFieldID];
		CoTaskMemFree(*ppwszStored);
		hr = SHStrDupW(pwz, ppwszStored);
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

HRESULT RdpCredential::GetCheckboxValue(DWORD dwFieldID, BOOL* pbChecked, PWSTR* ppwszLabel)
{
	UNREFERENCED_PARAMETER(dwFieldID);
	UNREFERENCED_PARAMETER(pbChecked);
	UNREFERENCED_PARAMETER(ppwszLabel);

	return E_NOTIMPL;
}

HRESULT RdpCredential::GetComboBoxValueCount(DWORD dwFieldID, DWORD* pcItems, DWORD* pdwSelectedItem)
{
	UNREFERENCED_PARAMETER(dwFieldID);
	UNREFERENCED_PARAMETER(pcItems);
	UNREFERENCED_PARAMETER(pdwSelectedItem);

	return E_NOTIMPL;
}

HRESULT RdpCredential::GetComboBoxValueAt(DWORD dwFieldID, DWORD dwItem, PWSTR* ppwszItem)
{
	UNREFERENCED_PARAMETER(dwFieldID);
	UNREFERENCED_PARAMETER(dwItem);
	UNREFERENCED_PARAMETER(ppwszItem);

	return E_NOTIMPL;
}

HRESULT RdpCredential::SetCheckboxValue(DWORD dwFieldID, BOOL bChecked)
{
	UNREFERENCED_PARAMETER(dwFieldID);
	UNREFERENCED_PARAMETER(bChecked);

	return E_NOTIMPL;
}

HRESULT RdpCredential::SetComboBoxSelectedValue(DWORD dwFieldId, DWORD dwSelectedItem)
{
	UNREFERENCED_PARAMETER(dwFieldId);
	UNREFERENCED_PARAMETER(dwSelectedItem);

	return E_NOTIMPL;
}

HRESULT RdpCredential::CommandLinkClicked(DWORD dwFieldID)
{
	UNREFERENCED_PARAMETER(dwFieldID);
	return E_NOTIMPL;
}

HRESULT RdpCredential::GetSerialization(CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
	CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs, PWSTR* ppwszOptionalStatusText,
	CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon)
{
	UNREFERENCED_PARAMETER(ppwszOptionalStatusText);
	UNREFERENCED_PARAMETER(pcpsiOptionalStatusIcon);

	HRESULT hr;

	WCHAR wsz[MAX_COMPUTERNAME_LENGTH+1];
	DWORD cch = ARRAYSIZE(wsz);
	if (GetComputerNameW(wsz, &cch))
	{
		PWSTR pwzProtectedPassword;

		hr = ProtectIfNecessaryAndCopyPassword(_rgFieldStrings[SFI_PASSWORD], _cpus, &pwzProtectedPassword);

		if (SUCCEEDED(hr))
		{
			KERB_INTERACTIVE_UNLOCK_LOGON kiul;

			hr = KerbInteractiveUnlockLogonInit(wsz, _rgFieldStrings[SFI_USERNAME], pwzProtectedPassword, _cpus, &kiul);

			if (SUCCEEDED(hr))
			{
				hr = KerbInteractiveUnlockLogonPack(kiul, &pcpcs->rgbSerialization, &pcpcs->cbSerialization);

				if (SUCCEEDED(hr))
				{
					ULONG ulAuthPackage;
					hr = RetrieveNegotiateAuthPackage(&ulAuthPackage);

					if (SUCCEEDED(hr))
					{
						pcpcs->ulAuthenticationPackage = ulAuthPackage;
						pcpcs->clsidCredentialProvider = CLSID_RdpProvider;

						*pcpgsr = CPGSR_RETURN_CREDENTIAL_FINISHED;
					}
				}
			}

			CoTaskMemFree(pwzProtectedPassword);
		}
	}
	else
	{
		DWORD dwErr = GetLastError();
		hr = HRESULT_FROM_WIN32(dwErr);
	}

	return hr;
}
struct REPORT_RESULT_STATUS_INFO
{
	NTSTATUS ntsStatus;
	NTSTATUS ntsSubstatus;
	PWSTR     pwzMessage;
	CREDENTIAL_PROVIDER_STATUS_ICON cpsi;
};

static const REPORT_RESULT_STATUS_INFO s_rgLogonStatusInfo[] =
{
	{ STATUS_LOGON_FAILURE, STATUS_SUCCESS, L"Incorrect password or username.", CPSI_ERROR, },
	{ STATUS_ACCOUNT_RESTRICTION, STATUS_ACCOUNT_DISABLED, L"The account is disabled.", CPSI_WARNING },
};

HRESULT RdpCredential::ReportResult(NTSTATUS ntsStatus, NTSTATUS ntsSubstatus,
	PWSTR* ppwszOptionalStatusText, CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon)
{
	*ppwszOptionalStatusText = NULL;
	*pcpsiOptionalStatusIcon = CPSI_NONE;

	DWORD dwStatusInfo = (DWORD)-1;

	for (DWORD i = 0; i < ARRAYSIZE(s_rgLogonStatusInfo); i++)
	{
		if (s_rgLogonStatusInfo[i].ntsStatus == ntsStatus && s_rgLogonStatusInfo[i].ntsSubstatus == ntsSubstatus)
		{
			dwStatusInfo = i;
			break;
		}
	}

	if ((DWORD)-1 != dwStatusInfo)
	{
		if (SUCCEEDED(SHStrDupW(s_rgLogonStatusInfo[dwStatusInfo].pwzMessage, ppwszOptionalStatusText)))
		{
			*pcpsiOptionalStatusIcon = s_rgLogonStatusInfo[dwStatusInfo].cpsi;
		}
	}

	if (!SUCCEEDED(HRESULT_FROM_NT(ntsStatus)))
	{
		if (_pCredProvCredentialEvents)
		{
			_pCredProvCredentialEvents->SetFieldString(this, SFI_PASSWORD, L"");
		}
	}

	return S_OK;
}
