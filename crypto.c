#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/core_names.h>
#include <openssl/param_build.h>
#include <openssl/x509.h>

// Utility function to print binary data in hex format
void print_hex(const char* label, const unsigned char* data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

// Utility function to handle OpenSSL errors
void handle_openssl_error() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

// Utility function to securely generate random bytes
void generate_random_bytes(unsigned char* buffer, size_t length) {
    if (RAND_bytes(buffer, length) != 1) {
        fprintf(stderr, "Error generating random bytes\n");
        handle_openssl_error();
    }
}

//---------------------------------------------------------------
// Symmetric Encryption (AES)
//---------------------------------------------------------------

// Encrypt data using AES-256-CBC
int aes_encrypt(const unsigned char* plaintext, int plaintext_len,
                const unsigned char* key, const unsigned char* iv,
                unsigned char* ciphertext, int* ciphertext_len) {
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        fprintf(stderr, "Error creating cipher context\n");
        handle_openssl_error();
        return 0;
    }
    
    // Initialize encryption operation
    if (EVP_EncryptInit_ex2(ctx, EVP_aes_256_cbc(), key, iv, NULL) != 1) {
        fprintf(stderr, "Error initializing encryption\n");
        EVP_CIPHER_CTX_free(ctx);
        handle_openssl_error();
        return 0;
    }
    
    int len;
    
    // Encrypt plaintext
    if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1) {
        fprintf(stderr, "Error encrypting data\n");
        EVP_CIPHER_CTX_free(ctx);
        handle_openssl_error();
        return 0;
    }
    
    *ciphertext_len = len;
    
    // Finalize encryption (handle any remaining blocks)
    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
        fprintf(stderr, "Error finalizing encryption\n");
        EVP_CIPHER_CTX_free(ctx);
        handle_openssl_error();
        return 0;
    }
    
    *ciphertext_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    return 1;
}

// Decrypt data using AES-256-CBC
int aes_decrypt(const unsigned char* ciphertext, int ciphertext_len,
                const unsigned char* key, const unsigned char* iv,
                unsigned char* plaintext, int* plaintext_len) {
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        fprintf(stderr, "Error creating cipher context\n");
        handle_openssl_error();
        return 0;
    }
    
    // Initialize decryption operation
    if (EVP_DecryptInit_ex2(ctx, EVP_aes_256_cbc(), key, iv, NULL) != 1) {
        fprintf(stderr, "Error initializing decryption\n");
        EVP_CIPHER_CTX_free(ctx);
        handle_openssl_error();
        return 0;
    }
    
    int len;
    
    // Decrypt ciphertext
    if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) != 1) {
        fprintf(stderr, "Error decrypting data\n");
        EVP_CIPHER_CTX_free(ctx);
        handle_openssl_error();
        return 0;
    }
    
    *plaintext_len = len;
    
    // Finalize decryption (handle any remaining blocks)
    if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) != 1) {
        fprintf(stderr, "Error finalizing decryption\n");
        EVP_CIPHER_CTX_free(ctx);
        handle_openssl_error();
        return 0;
    }
    
    *plaintext_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    return 1;
}

// Encrypt data using AES-256-GCM (authenticated encryption)
int aes_gcm_encrypt(const unsigned char* plaintext, int plaintext_len,
                    const unsigned char* aad, int aad_len,
                    const unsigned char* key, const unsigned char* iv, int iv_len,
                    unsigned char* ciphertext, int* ciphertext_len,
                    unsigned char* tag, int tag_len) {
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        fprintf(stderr, "Error creating cipher context\n");
        handle_openssl_error();
        return 0;
    }
    
    // Initialize encryption operation
    if (EVP_EncryptInit_ex2(ctx, EVP_aes_256_gcm(), key, iv, NULL) != 1) {
        fprintf(stderr, "Error initializing GCM encryption\n");
        EVP_CIPHER_CTX_free(ctx);
        handle_openssl_error();
        return 0;
    }
    
    // Set IV length (GCM allows custom IV lengths)
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL) != 1) {
        fprintf(stderr, "Error setting IV length\n");
        EVP_CIPHER_CTX_free(ctx);
        handle_openssl_error();
        return 0;
    }
    
    int len;
    
    // Provide AAD (additional authenticated data) if present
    if (aad && aad_len > 0) {
        if (EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len) != 1) {
            fprintf(stderr, "Error processing AAD\n");
            EVP_CIPHER_CTX_free(ctx);
            handle_openssl_error();
            return 0;
        }
    }
    
    // Encrypt plaintext
    if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1) {
        fprintf(stderr, "Error encrypting data\n");
        EVP_CIPHER_CTX_free(ctx);
        handle_openssl_error();
        return 0;
    }
    
    *ciphertext_len = len;
    
    // Finalize encryption
    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
        fprintf(stderr, "Error finalizing encryption\n");
        EVP_CIPHER_CTX_free(ctx);
        handle_openssl_error();
        return 0;
    }
    
    *ciphertext_len += len;
    
    // Get the authentication tag
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, tag_len, tag) != 1) {
        fprintf(stderr, "Error getting authentication tag\n");
        EVP_CIPHER_CTX_free(ctx);
        handle_openssl_error();
        return 0;
    }
    
    EVP_CIPHER_CTX_free(ctx);
    return 1;
}

// Decrypt data using AES-256-GCM (authenticated encryption)
int aes_gcm_decrypt(const unsigned char* ciphertext, int ciphertext_len,
                    const unsigned char* aad, int aad_len,
                    const unsigned char* tag, int tag_len,
                    const unsigned char* key, const unsigned char* iv, int iv_len,
                    unsigned char* plaintext, int* plaintext_len) {
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        fprintf(stderr, "Error creating cipher context\n");
        handle_openssl_error();
        return 0;
    }
    
    // Initialize decryption operation
    if (EVP_DecryptInit_ex2(ctx, EVP_aes_256_gcm(), key, iv, NULL) != 1) {
        fprintf(stderr, "Error initializing GCM decryption\n");
        EVP_CIPHER_CTX_free(ctx);
        handle_openssl_error();
        return 0;
    }
    
    // Set IV length
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL) != 1) {
        fprintf(stderr, "Error setting IV length\n");
        EVP_CIPHER_CTX_free(ctx);
        handle_openssl_error();
        return 0;
    }
    
    int len;
    
    // Provide AAD
    if (aad && aad_len > 0) {
        if (EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len) != 1) {
            fprintf(stderr, "Error processing AAD\n");
            EVP_CIPHER_CTX_free(ctx);
            handle_openssl_error();
            return 0;
        }
    }
    
    // Decrypt ciphertext
    if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) != 1) {
        fprintf(stderr, "Error decrypting data\n");
        EVP_CIPHER_CTX_free(ctx);
        handle_openssl_error();
        return 0;
    }
    
    *plaintext_len = len;
    
    // Set expected tag value
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag_len, (void*)tag) != 1) {
        fprintf(stderr, "Error setting authentication tag\n");
        EVP_CIPHER_CTX_free(ctx);
        handle_openssl_error();
        return 0;
    }
    
    // Finalize decryption and verify tag
    if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) != 1) {
        fprintf(stderr, "Error finalizing decryption (tag authentication failed)\n");
        EVP_CIPHER_CTX_free(ctx);
        return 0; // Authentication failed
    }
    
    *plaintext_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    return 1;
}

//---------------------------------------------------------------
// Asymmetric Encryption (RSA)
//---------------------------------------------------------------

// Generate an RSA key pair and save to files
void generate_rsa_key_pair(const char* private_key_file, const char* public_key_file, int key_bits) {
    EVP_PKEY_CTX* ctx = NULL;
    EVP_PKEY* pkey = NULL;
    FILE* private_fp = NULL;
    FILE* public_fp = NULL;
    
    // Create context for key generation
    ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (!ctx) {
        fprintf(stderr, "Error creating EVP_PKEY_CTX\n");
        handle_openssl_error();
        goto cleanup;
    }
    
    // Initialize key generation
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        fprintf(stderr, "Error initializing key generation\n");
        handle_openssl_error();
        goto cleanup;
    }
    
    // Set RSA key bits
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, key_bits) <= 0) {
        fprintf(stderr, "Error setting RSA key size\n");
        handle_openssl_error();
        goto cleanup;
    }
    
    // Generate key pair
    printf("Generating RSA key pair (%d bits)...\n", key_bits);
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        fprintf(stderr, "Error generating RSA key pair\n");
        handle_openssl_error();
        goto cleanup;
    }
    
    printf("Key generation complete\n");
    
    // Save private key
    private_fp = fopen(private_key_file, "wb");
    if (!private_fp) {
        fprintf(stderr, "Error opening private key file for writing\n");
        goto cleanup;
    }
    
    if (PEM_write_PrivateKey(private_fp, pkey, NULL, NULL, 0, NULL, NULL) != 1) {
        fprintf(stderr, "Error writing private key to file\n");
        handle_openssl_error();
        goto cleanup;
    }
    
    printf("Private key saved to %s\n", private_key_file);
    
    // Save public key
    public_fp = fopen(public_key_file, "wb");
    if (!public_fp) {
        fprintf(stderr, "Error opening public key file for writing\n");
        goto cleanup;
    }
    
    if (PEM_write_PUBKEY(public_fp, pkey) != 1) {
        fprintf(stderr, "Error writing public key to file\n");
        handle_openssl_error();
        goto cleanup;
    }
    
    printf("Public key saved to %s\n", public_key_file);
    
cleanup:
    if (ctx) EVP_PKEY_CTX_free(ctx);
    if (pkey) EVP_PKEY_free(pkey);
    if (private_fp) fclose(private_fp);
    if (public_fp) fclose(public_fp);
}

// Load a public key from file
EVP_PKEY* load_public_key(const char* public_key_file) {
    FILE* fp = fopen(public_key_file, "rb");
    if (!fp) {
        fprintf(stderr, "Error opening public key file: %s\n", public_key_file);
        return NULL;
    }
    
    EVP_PKEY* pkey = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
    fclose(fp);
    
    if (!pkey) {
        fprintf(stderr, "Error reading public key\n");
        handle_openssl_error();
        return NULL;
    }
    
    return pkey;
}

// Load a private key from file
EVP_PKEY* load_private_key(const char* private_key_file) {
    FILE* fp = fopen(private_key_file, "rb");
    if (!fp) {
        fprintf(stderr, "Error opening private key file: %s\n", private_key_file);
        return NULL;
    }
    
    EVP_PKEY* pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    fclose(fp);
    
    if (!pkey) {
        fprintf(stderr, "Error reading private key\n");
        handle_openssl_error();
        return NULL;
    }
    
    return pkey;
}

// RSA encrypt using public key
int rsa_public_encrypt(const unsigned char* plaintext, size_t plaintext_len,
                      EVP_PKEY* pkey, unsigned char* ciphertext, size_t* ciphertext_len) {
    
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (!ctx) {
        fprintf(stderr, "Error creating encryption context\n");
        handle_openssl_error();
        return 0;
    }
    
    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        fprintf(stderr, "Error initializing encryption\n");
        handle_openssl_error();
        EVP_PKEY_CTX_free(ctx);
        return 0;
    }
    
    // Set padding mode to OAEP (recommended)
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        fprintf(stderr, "Error setting padding mode\n");
        handle_openssl_error();
        EVP_PKEY_CTX_free(ctx);
        return 0;
    }
    
    // Determine buffer size needed
    if (EVP_PKEY_encrypt(ctx, NULL, ciphertext_len, plaintext, plaintext_len) <= 0) {
        fprintf(stderr, "Error determining output length\n");
        handle_openssl_error();
        EVP_PKEY_CTX_free(ctx);
        return 0;
    }
    
    // Encrypt data
    if (EVP_PKEY_encrypt(ctx, ciphertext, ciphertext_len, plaintext, plaintext_len) <= 0) {
        fprintf(stderr, "Error encrypting data\n");
        handle_openssl_error();
        EVP_PKEY_CTX_free(ctx);
        return 0;
    }
    
    EVP_PKEY_CTX_free(ctx);
    return 1;
}

// RSA decrypt using private key
int rsa_private_decrypt(const unsigned char* ciphertext, size_t ciphertext_len,
                       EVP_PKEY* pkey, unsigned char* plaintext, size_t* plaintext_len) {
    
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (!ctx) {
        fprintf(stderr, "Error creating decryption context\n");
        handle_openssl_error();
        return 0;
    }
    
    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        fprintf(stderr, "Error initializing decryption\n");
        handle_openssl_error();
        EVP_PKEY_CTX_free(ctx);
        return 0;
    }
    
    // Set padding mode to OAEP (must match encryption)
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        fprintf(stderr, "Error setting padding mode\n");
        handle_openssl_error();
        EVP_PKEY_CTX_free(ctx);
        return 0;
    }
    
    // Determine buffer size needed
    if (EVP_PKEY_decrypt(ctx, NULL, plaintext_len, ciphertext, ciphertext_len) <= 0) {
        fprintf(stderr, "Error determining output length\n");
        handle_openssl_error();
        EVP_PKEY_CTX_free(ctx);
        return 0;
    }
    
    // Decrypt data
    if (EVP_PKEY_decrypt(ctx, plaintext, plaintext_len, ciphertext, ciphertext_len) <= 0) {
        fprintf(stderr, "Error decrypting data\n");
        handle_openssl_error();
        EVP_PKEY_CTX_free(ctx);
        return 0;
    }
    
    EVP_PKEY_CTX_free(ctx);
    return 1;
}

//---------------------------------------------------------------
// Digital Signatures
//---------------------------------------------------------------

// Sign data using a private key (with SHA-256 digest)
int sign_data(const unsigned char* data, size_t data_len,
              unsigned char* signature, size_t* signature_len,
              EVP_PKEY* pkey) {
    
    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        fprintf(stderr, "Error creating message digest context\n");
        handle_openssl_error();
        return 0;
    }
    
    // Initialize signing operation with SHA-256
    if (EVP_DigestSignInit_ex(md_ctx, NULL, "SHA256", NULL, NULL, pkey, NULL) != 1) {
        fprintf(stderr, "Error initializing signing operation\n");
        handle_openssl_error();
        EVP_MD_CTX_free(md_ctx);
        return 0;
    }
    
    // Add message data
    if (EVP_DigestSignUpdate(md_ctx, data, data_len) != 1) {
        fprintf(stderr, "Error adding message data\n");
        handle_openssl_error();
        EVP_MD_CTX_free(md_ctx);
        return 0;
    }
    
    // Get signature length
    if (EVP_DigestSignFinal(md_ctx, NULL, signature_len) != 1) {
        fprintf(stderr, "Error determining signature length\n");
        handle_openssl_error();
        EVP_MD_CTX_free(md_ctx);
        return 0;
    }
    
    // Get actual signature
    if (EVP_DigestSignFinal(md_ctx, signature, signature_len) != 1) {
        fprintf(stderr, "Error getting signature\n");
        handle_openssl_error();
        EVP_MD_CTX_free(md_ctx);
        return 0;
    }
    
    EVP_MD_CTX_free(md_ctx);
    return 1;
}

// Verify a signature using a public key
int verify_signature(const unsigned char* data, size_t data_len,
                    const unsigned char* signature, size_t signature_len,
                    EVP_PKEY* pkey) {
    
    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        fprintf(stderr, "Error creating message digest context\n");
        handle_openssl_error();
        return 0;
    }
    
    // Initialize verification operation with SHA-256
    if (EVP_DigestVerifyInit_ex(md_ctx, NULL, "SHA256", NULL, NULL, pkey, NULL) != 1) {
        fprintf(stderr, "Error initializing verification operation\n");
        handle_openssl_error();
        EVP_MD_CTX_free(md_ctx);
        return 0;
    }
    
    // Add message data
    if (EVP_DigestVerifyUpdate(md_ctx, data, data_len) != 1) {
        fprintf(stderr, "Error adding message data\n");
        handle_openssl_error();
        EVP_MD_CTX_free(md_ctx);
        return 0;
    }
    
    // Verify signature
    int result = EVP_DigestVerifyFinal(md_ctx, signature, signature_len);
    
    EVP_MD_CTX_free(md_ctx);
    
    if (result != 1) {
        if (result == 0) {
            fprintf(stderr, "Signature verification failed (invalid signature)\n");
        } else {
            fprintf(stderr, "Error verifying signature\n");
            handle_openssl_error();
        }
        return 0;
    }
    
    return 1; // Verification successful
}

//---------------------------------------------------------------
// Hash Functions
//---------------------------------------------------------------

// Compute a SHA-256 hash of data
int compute_sha256(const unsigned char* data, size_t data_len,
                  unsigned char* hash, unsigned int* hash_len) {
    
    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        fprintf(stderr, "Error creating message digest context\n");
        handle_openssl_error();
        return 0;
    }
    
    if (EVP_DigestInit_ex2(md_ctx, EVP_sha256(), NULL) != 1) {
        fprintf(stderr, "Error initializing digest\n");
        handle_openssl_error();
        EVP_MD_CTX_free(md_ctx);
        return 0;
    }
    
    if (EVP_DigestUpdate(md_ctx, data, data_len) != 1) {
        fprintf(stderr, "Error updating digest\n");
        handle_openssl_error();
        EVP_MD_CTX_free(md_ctx);
        return 0;
    }
    
    if (EVP_DigestFinal_ex(md_ctx, hash, hash_len) != 1) {
        fprintf(stderr, "Error finalizing digest\n");
        handle_openssl_error();
        EVP_MD_CTX_free(md_ctx);
        return 0;
    }
    
    EVP_MD_CTX_free(md_ctx);
    return 1;
}

// Compute a HMAC-SHA256 (keyed hash) of data using the EVP_MAC API
int compute_hmac_sha256(const unsigned char* data, size_t data_len,
                        const unsigned char* key, size_t key_len,
                        unsigned char* hmac, size_t* hmac_len) {
    
    int ret = 0;
    EVP_MAC* mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
    if (!mac) {
        fprintf(stderr, "Error fetching HMAC\n");
        handle_openssl_error();
        return 0;
    }
    
    EVP_MAC_CTX* mctx = EVP_MAC_CTX_new(mac);
    if (!mctx) {
        fprintf(stderr, "Error creating MAC context\n");
        handle_openssl_error();
        EVP_MAC_free(mac);
        return 0;
    }
    
    OSSL_PARAM params[2];
    params[0] = OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST, "SHA256", 0);
    params[1] = OSSL_PARAM_construct_end();
    
    // Initialize MAC operation
    if (EVP_MAC_init(mctx, key, key_len, params) != 1) {
        fprintf(stderr, "Error initializing HMAC\n");
        handle_openssl_error();
        goto cleanup;
    }
    
    // Process data
    if (EVP_MAC_update(mctx, data, data_len) != 1) {
        fprintf(stderr, "Error updating HMAC\n");
        handle_openssl_error();
        goto cleanup;
    }
    
    // Finalize and get result
    if (EVP_MAC_final(mctx, hmac, hmac_len, *hmac_len) != 1) {
        fprintf(stderr, "Error finalizing HMAC\n");
        handle_openssl_error();
        goto cleanup;
    }
    
    ret = 1;
    
cleanup:
    EVP_MAC_CTX_free(mctx);
    EVP_MAC_free(mac);
    return ret;
}

//---------------------------------------------------------------
// Helper Functions
//---------------------------------------------------------------

// Derive a key from a password using PBKDF2
int derive_key_from_password(const char* password, const unsigned char* salt, size_t salt_len,
                            unsigned char* key, size_t key_len, int iterations) {
    
    EVP_KDF* kdf = EVP_KDF_fetch(NULL, "PBKDF2", NULL);
    if (!kdf) {
        fprintf(stderr, "Error fetching KDF\n");
        handle_openssl_error();
        return 0;
    }
    
    EVP_KDF_CTX* kctx = EVP_KDF_CTX_new(kdf);
    if (!kctx) {
        fprintf(stderr, "Error creating KDF context\n");
        handle_openssl_error();
        EVP_KDF_free(kdf);
        return 0;
    }
    
    OSSL_PARAM params[5], *p = params;
    
    char iter_str[20];
    sprintf(iter_str, "%d", iterations);
    
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_PASSWORD, 
                                           (void*)password, strlen(password));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, 
                                           (void*)salt, salt_len);
    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, "SHA256", 0);
    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_ITER, iter_str, 0);
    *p = OSSL_PARAM_construct_end();
    
    if (EVP_KDF_derive(kctx, key, key_len, params) != 1) {
        fprintf(stderr, "Error deriving key\n");
        handle_openssl_error();
        EVP_KDF_CTX_free(kctx);
        EVP_KDF_free(kdf);
        return 0;
    }
    
    EVP_KDF_CTX_free(kctx);
    EVP_KDF_free(kdf);
    return 1;
}

// Base64 encode binary data
char* base64_encode(const unsigned char* input, int length) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    
    // Ensure no newlines in output
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    
    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);
    
    char* buff = (char*)malloc(bptr->length + 1);
    memcpy(buff, bptr->data, bptr->length);
    buff[bptr->length] = 0;
    
    BIO_free_all(b64);
    
    return buff;
}

// Base64 decode data to binary
unsigned char* base64_decode(const char* input, int* length) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new_mem_buf(input, -1);
    
    // Ensure no newlines in input
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    
    bmem = BIO_push(b64, bmem);
    
    // Allocate buffer for decoded data (approx 3/4 of encoded length)
    unsigned char* buffer = (unsigned char*)malloc(strlen(input) * 3 / 4 + 1);
    *length = BIO_read(bmem, buffer, strlen(input));
    
    BIO_free_all(bmem);
    
    return buffer;
}

//---------------------------------------------------------------
// Demo Functions
//---------------------------------------------------------------

void demo_aes_encryption() {
    printf("\n=== AES Encryption Demo ===\n");
    
    // Message to encrypt
    const char* plaintext = "This is a secret message for AES encryption.";
    printf("Original text: %s\n", plaintext);
    
    // Generate a random AES-256 key (32 bytes)
    unsigned char key[32];
    generate_random_bytes(key, sizeof(key));
    print_hex("AES Key", key, sizeof(key));
    
    // Generate a random IV (16 bytes for AES block size)
    unsigned char iv[16];
    generate_random_bytes(iv, sizeof(iv));
    print_hex("IV", iv, sizeof(iv));
    
    // Encrypt using AES-256-CBC
    unsigned char ciphertext[1024];
    int ciphertext_len;
    
    if (!aes_encrypt((unsigned char*)plaintext, strlen(plaintext),
                    key, iv, ciphertext, &ciphertext_len)) {
        fprintf(stderr, "AES encryption failed\n");
        return;
    }
    
    printf("Ciphertext length: %d bytes\n", ciphertext_len);
    print_hex("Ciphertext", ciphertext, ciphertext_len);
    
    // Decrypt
    unsigned char decrypted[1024];
    int decrypted_len;
    
    if (!aes_decrypt(ciphertext, ciphertext_len,
                    key, iv, decrypted, &decrypted_len)) {
        fprintf(stderr, "AES decryption failed\n");
        return;
    }
    
    // Null-terminate the decrypted text
    decrypted[decrypted_len] = '\0';
    printf("Decrypted text: %s\n", decrypted);
}

void demo_aes_gcm_encryption() {
    printf("\n=== AES-GCM Authenticated Encryption Demo ===\n");
    
    // Message to encrypt
    const char* plaintext = "This is a secret message for AES-GCM encryption.";
    printf("Original text: %s\n", plaintext);
    
    // Additional data to authenticate but not encrypt
    const char* aad = "Additional authenticated data";
    printf("Additional authenticated data: %s\n", aad);
    
    // Generate a random AES-256 key (32 bytes)
    unsigned char key[32];
    generate_random_bytes(key, sizeof(key));
    print_hex("AES Key", key, sizeof(key));
    
    // Generate a random nonce/IV (12 bytes is recommended for GCM)
    unsigned char iv[12];
    generate_random_bytes(iv, sizeof(iv));
    print_hex("Nonce/IV", iv, sizeof(iv));
    
    // Encrypt and authenticate using AES-256-GCM
    unsigned char ciphertext[1024];
    int ciphertext_len;
    unsigned char tag[16]; // Authentication tag (16 bytes)
    
    if (!aes_gcm_encrypt((unsigned char*)plaintext, strlen(plaintext),
                        (unsigned char*)aad, strlen(aad),
                        key, iv, sizeof(iv),
                        ciphertext, &ciphertext_len,
                        tag, sizeof(tag))) {
        fprintf(stderr, "AES-GCM encryption failed\n");
        return;
    }
    
    printf("Ciphertext length: %d bytes\n", ciphertext_len);
    print_hex("Ciphertext", ciphertext, ciphertext_len);
    print_hex("Auth Tag", tag, sizeof(tag));
    
    // Decrypt and verify
    unsigned char decrypted[1024];
    int decrypted_len;
    
    if (!aes_gcm_decrypt(ciphertext, ciphertext_len,
                        (unsigned char*)aad, strlen(aad),
                        tag, sizeof(tag),
                        key, iv, sizeof(iv),
                        decrypted, &decrypted_len)) {
        fprintf(stderr, "AES-GCM decryption failed (authentication failed)\n");
        return;
    }
    
    // Null-terminate the decrypted text
    decrypted[decrypted_len] = '\0';
    printf("Decrypted text: %s\n", decrypted);
    
    // Try with a tampered tag to demonstrate authentication
    printf("\nTesting with a tampered authentication tag...\n");
    tag[0] ^= 1; // Flip a bit in the tag
    
    if (!aes_gcm_decrypt(ciphertext, ciphertext_len,
                        (unsigned char*)aad, strlen(aad),
                        tag, sizeof(tag),
                        key, iv, sizeof(iv),
                        decrypted, &decrypted_len)) {
        printf("Decryption correctly failed with tampered authentication tag\n");
    } else {
        printf("WARNING: Decryption succeeded with tampered tag! This should not happen.\n");
    }
}

void demo_rsa_encryption() {
    printf("\n=== RSA Encryption Demo ===\n");
    
    // Generate RSA key pair (2048 bits)
    const char* private_key_file = "private_key.pem";
    const char* public_key_file = "public_key.pem";
    
    generate_rsa_key_pair(private_key_file, public_key_file, 2048);
    
    // Load the key pair
    EVP_PKEY* private_key = load_private_key(private_key_file);
    EVP_PKEY* public_key = load_public_key(public_key_file);
    
    if (!private_key || !public_key) {
        fprintf(stderr, "Error loading RSA keys\n");
        if (private_key) EVP_PKEY_free(private_key);
        if (public_key) EVP_PKEY_free(public_key);
        return;
    }
    
    // Message to encrypt
    const char* plaintext = "This is a secret message for RSA encryption.";
    printf("Original text: %s\n", plaintext);
    
    // Allocate buffer for encrypted data
    unsigned char ciphertext[512]; // Enough for 2048-bit RSA
    size_t ciphertext_len = sizeof(ciphertext);
    
    // Encrypt with public key
    if (!rsa_public_encrypt((unsigned char*)plaintext, strlen(plaintext),
                          public_key, ciphertext, &ciphertext_len)) {
        fprintf(stderr, "RSA encryption failed\n");
        EVP_PKEY_free(private_key);
        EVP_PKEY_free(public_key);
        return;
    }
    
    printf("Ciphertext length: %zu bytes\n", ciphertext_len);
    print_hex("Ciphertext", ciphertext, ciphertext_len);
    
    // Allocate buffer for decrypted data
    unsigned char decrypted[512];
    size_t decrypted_len = sizeof(decrypted);
    
    // Decrypt with private key
    if (!rsa_private_decrypt(ciphertext, ciphertext_len,
                            private_key, decrypted, &decrypted_len)) {
        fprintf(stderr, "RSA decryption failed\n");
        EVP_PKEY_free(private_key);
        EVP_PKEY_free(public_key);
        return;
    }
    
    // Null-terminate the decrypted text
    decrypted[decrypted_len] = '\0';
    printf("Decrypted text: %s\n", decrypted);
    
    EVP_PKEY_free(private_key);
    EVP_PKEY_free(public_key);
}

void demo_digital_signature() {
    printf("\n=== Digital Signature Demo ===\n");
    
    // Load the key pair (assuming the keys are already generated)
    const char* private_key_file = "private_key.pem";
    const char* public_key_file = "public_key.pem";
    
    EVP_PKEY* private_key = load_private_key(private_key_file);
    EVP_PKEY* public_key = load_public_key(public_key_file);
    
    if (!private_key || !public_key) {
        fprintf(stderr, "Error loading RSA keys\n");
        if (private_key) EVP_PKEY_free(private_key);
        if (public_key) EVP_PKEY_free(public_key);
        return;
    }
    
    // Message to sign
    const char* message = "This is a message to be digitally signed.";
    printf("Original message: %s\n", message);
    
    // Allocate buffer for signature
    unsigned char signature[512]; // Plenty of space for 2048-bit RSA
    size_t signature_len = sizeof(signature);
    
    // Sign the message
    if (!sign_data((unsigned char*)message, strlen(message),
                 signature, &signature_len, private_key)) {
        fprintf(stderr, "Signing failed\n");
        EVP_PKEY_free(private_key);
        EVP_PKEY_free(public_key);
        return;
    }
    
    printf("Signature length: %zu bytes\n", signature_len);
    print_hex("Signature", signature, signature_len);
    
    // Verify the signature
    if (verify_signature((unsigned char*)message, strlen(message),
                       signature, signature_len, public_key)) {
        printf("Signature verification SUCCESSFUL\n");
    } else {
        printf("Signature verification FAILED\n");
    }
    
    // Test with a tampered message
    printf("\nTesting with a tampered message...\n");
    const char* tampered_message = "This is a message that has been tampered with!";
    printf("Tampered message: %s\n", tampered_message);
    
    if (verify_signature((unsigned char*)tampered_message, strlen(tampered_message),
                       signature, signature_len, public_key)) {
        printf("WARNING: Tampered message verification succeeded! This should not happen.\n");
    } else {
        printf("Tampered message verification correctly failed\n");
    }
    
    EVP_PKEY_free(private_key);
    EVP_PKEY_free(public_key);
}

void demo_hash_functions() {
    printf("\n=== Hash Function Demo ===\n");
    
    // Data to hash
    const char* data = "This is data to be hashed.";
    printf("Original data: %s\n", data);
    
    // Compute SHA-256 hash
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    
    if (!compute_sha256((unsigned char*)data, strlen(data), hash, &hash_len)) {
        fprintf(stderr, "SHA-256 hashing failed\n");
        return;
    }
    
    printf("SHA-256 hash length: %u bytes\n", hash_len);
    print_hex("SHA-256 hash", hash, hash_len);
    
    // Compute HMAC-SHA256
    const char* hmac_key = "secret_key_for_hmac";
    unsigned char hmac[EVP_MAX_MD_SIZE];
    size_t hmac_len = sizeof(hmac);
    
    if (!compute_hmac_sha256((unsigned char*)data, strlen(data),
                            (unsigned char*)hmac_key, strlen(hmac_key),
                            hmac, &hmac_len)) {
        fprintf(stderr, "HMAC-SHA256 computation failed\n");
        return;
    }
    
    printf("HMAC-SHA256 length: %zu bytes\n", hmac_len);
    print_hex("HMAC-SHA256", hmac, hmac_len);
}

void demo_key_derivation() {
    printf("\n=== Key Derivation Demo ===\n");
    
    // Password for key derivation
    const char* password = "user_password";
    printf("Password: %s\n", password);
    
    // Generate a random salt
    unsigned char salt[16];
    generate_random_bytes(salt, sizeof(salt));
    print_hex("Salt", salt, sizeof(salt));
    
    // Derive a 32-byte key (256 bits) using PBKDF2
    unsigned char derived_key[32];
    int iterations = 10000; // Number of iterations for PBKDF2
    
    if (!derive_key_from_password(password, salt, sizeof(salt),
                                derived_key, sizeof(derived_key),
                                iterations)) {
        fprintf(stderr, "Key derivation failed\n");
        return;
    }
    
    printf("Derived key length: %zu bytes\n", sizeof(derived_key));
    print_hex("Derived key", derived_key, sizeof(derived_key));
}

void demo_base64() {
    printf("\n=== Base64 Encoding Demo ===\n");
    
    // Binary data to encode
    unsigned char binary_data[] = {0x01, 0x02, 0x03, 0xF1, 0xF2, 0xF3, 0xAA, 0xBB, 0xCC, 0xDD};
    print_hex("Original binary data", binary_data, sizeof(binary_data));
    
    // Encode to Base64
    char* base64_str = base64_encode(binary_data, sizeof(binary_data));
    printf("Base64 encoded: %s\n", base64_str);
    
    // Decode from Base64
    int decoded_len;
    unsigned char* decoded_data = base64_decode(base64_str, &decoded_len);
    
    printf("Decoded length: %d bytes\n", decoded_len);
    print_hex("Decoded data", decoded_data, decoded_len);
    
    free(base64_str);
    free(decoded_data);
}

//---------------------------------------------------------------
// Main Function
//---------------------------------------------------------------

int main(int argc, char* argv[]) {
    printf("OpenSSL Cryptography Demo\n");
    printf("========================\n");
    
    // Run the demos
    demo_aes_encryption();
    demo_aes_gcm_encryption();
    demo_rsa_encryption();
    demo_digital_signature();
    demo_hash_functions();
    demo_key_derivation();
    demo_base64();
    
    return 0;
}