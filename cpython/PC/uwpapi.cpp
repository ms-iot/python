#include "Python.h"

#ifdef MS_UWP

#include <Windows.h>

#define X509_ASN_ENC "x509_asn"

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::System::UserProfile;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::Security::Cryptography;
using namespace Windows::Security::Cryptography::Certificates;

extern "C" {
    
    void win32_urandom(unsigned char *buffer, Py_ssize_t size, int raise)
    {
        IBuffer^ data = CryptographicBuffer::GenerateRandom(size);
        Array<unsigned char>^ data2;
        CryptographicBuffer::CopyToByteArray(data, &data2);
        for(int i=0; i < size; i++)
            buffer[i] = data2[i];
    }

    /*BOOL*/ int uwp_startfile(const wchar_t *operation, const wchar_t *path)
    {
        /* TODO: Implement launcher */
        return FALSE;
    }

    size_t uwp_getinstallpath(wchar_t *buffer, size_t cch)
    {
        try
        {
            String^ path = Package::Current->InstalledLocation->Path;
            wcscpy_s(buffer, cch, path->Data());
            return path->Length();
        }
        catch (Exception^)
        {
            return 0;
        }
    }

    static bool set_item(PyObject *d, const wchar_t *name, String^ (*value_func)())
    {
        PyObject *valueobj;
        try
        {
            auto value = value_func();
            valueobj = PyUnicode_FromWideChar(value->Data(), -1);
        }
        catch (Exception^)
        {
            valueobj = PyUnicode_FromString("");
        }
    
        if (!valueobj)
            return false;

        auto nameobj = PyUnicode_FromWideChar(name, -1);
        if (!nameobj)
        {
            Py_DECREF(valueobj);
            return false;
        }

        bool success = PyDict_SetItem(d, nameobj, valueobj) == 0;
        Py_DECREF(nameobj);
        Py_DECREF(valueobj);
        return success;
    }

    PyObject * uwp_defaultenviron()
    {
        auto d = PyDict_New();

        if (d != nullptr &&
            set_item(d, L"INSTALLPATH", [] { return Package::Current->InstalledLocation->Path; }) &&
            set_item(d, L"APPDATA", [] { return ApplicationData::Current->RoamingFolder->Path; }) &&
            set_item(d, L"LOCALAPPDATA", [] { return ApplicationData::Current->LocalFolder->Path; }) &&
            set_item(d, L"TEMP", [] { return ApplicationData::Current->TemporaryFolder->Path; }) &&
            set_item(d, L"TMP", [] { return ApplicationData::Current->TemporaryFolder->Path; })
            )
            return d;

        Py_DECREF(d);
        return nullptr;
    }

    PyObject * uwp_enumcertificates()
    {
        PyObject *result = NULL;
        PyObject *keyusage = NULL, *cert = NULL, *enc = NULL, *tup = NULL;
        auto asyncResult = CertificateStores::FindAllAsync();
        auto certs = asyncResult->GetResults();

        result = PyList_New(0);
        if (result == NULL)
        {
            return result;
        }

        for (auto itr = certs->First(); itr->HasCurrent; itr->MoveNext())
        {
            auto certificate = itr->Current;
            auto encodedCert = certificate->GetCertificateBlob();

            byte* buffer = reinterpret_cast<byte*>(PyMem_Malloc(encodedCert->Length));
            if (!buffer) {
                Py_CLEAR(result);
                result = NULL;
                break;
            }

            auto bufferArray = ArrayReference<byte>(buffer, encodedCert->Length);

            auto reader = DataReader::FromBuffer(encodedCert);

            reader->ReadBytes(bufferArray);

            cert = PyBytes_FromStringAndSize((const char *)buffer, encodedCert->Length);
            PyMem_Free(buffer);
            buffer = NULL;

            if (!cert) {
                Py_CLEAR(result);
                result = NULL;
                break;
            }

            keyusage = PySet_New(NULL);
            for (auto kuitr = certificate->EnhancedKeyUsages->First(); kuitr->HasCurrent; kuitr->MoveNext())
            {
                PySet_Add(keyusage, PyUnicode_FromWideChar(kuitr->Current->Data(), kuitr->Current->Length()));
            }

            enc = PyUnicode_InternFromString(X509_ASN_ENC);
            if ((tup = PyTuple_New(3)) == NULL) {
                Py_CLEAR(result);
                break;
            }

            PyTuple_SET_ITEM(tup, 0, cert);

            PyTuple_SET_ITEM(tup, 1, enc);

            PyTuple_SET_ITEM(tup, 1, keyusage);

            PyList_Append(result, tup);

            cert = NULL;
            enc = NULL;
            keyusage = NULL;
        }

        return result;
    }
}

#endif
