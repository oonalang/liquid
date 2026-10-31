#include "../src/main/jni/Includes/AndroidCurlTrust.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unistd.h>

int main(int argc, char** argv) {
    assert(argc == 2);
    SSL_CTX* ctx = SSL_CTX_new(TLS_method());
    assert(ctx);
    X509_STORE* store = SSL_CTX_get_cert_store(ctx);
    assert(store);
    assert(ethnir_tls::LoadCertificateDirectory(store, "/no/such/trust/store") == 0);
    assert(ethnir_tls::LoadCertificateDirectory(store, argv[1]) > 0);
    assert(ethnir_tls::LoadCertificateDirectory(store, argv[1]) > 0);
    SSL_CTX_free(ctx);
    return 0;
}
