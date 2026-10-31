#pragma once

#include <curl/curl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <string>

namespace ethnir_tls {

inline unsigned LoadCertificateDirectory(X509_STORE* store, const char* directory) {
    if (!store || !directory) return 0;
    DIR* dir = opendir(directory);
    if (!dir) return 0;
    unsigned parsed = 0;
    unsigned scanned = 0;
    while (dirent* entry = readdir(dir)) {
        if (++scanned > 1024) break;
        if (entry->d_name[0] == '.') continue;
        const std::string path = std::string(directory) + "/" + entry->d_name;
        struct stat info{};
        if (stat(path.c_str(), &info) != 0 || !S_ISREG(info.st_mode) || info.st_size > 65536) continue;
        BIO* file = BIO_new_file(path.c_str(), "r");
        if (!file) {
            ERR_clear_error();
            continue;
        }
        X509* certificate = PEM_read_bio_X509(file, nullptr, nullptr, nullptr);
        if (certificate) {
            ++parsed;
            if (X509_STORE_add_cert(store, certificate) != 1) ERR_clear_error();
            X509_free(certificate);
        } else {
            ERR_clear_error();
        }
        BIO_free(file);
    }
    closedir(dir);
    return parsed;
}

inline CURLcode InstallAndroidTrust(CURL*, void* sslContext, void*) {
    if (!sslContext) return CURLE_SSL_CERTPROBLEM;
    X509_STORE* store = SSL_CTX_get_cert_store(static_cast<SSL_CTX*>(sslContext));
    if (!store) return CURLE_SSL_CERTPROBLEM;
    const unsigned count =
        LoadCertificateDirectory(store, "/apex/com.android.conscrypt/cacerts") +
        LoadCertificateDirectory(store, "/system/etc/security/cacerts");
    return count ? CURLE_OK : CURLE_SSL_CERTPROBLEM;
}

}
