package com.foundry.ide.auth

import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import kotlinx.coroutines.*
import java.io.File
import java.security.MessageDigest
import java.security.SecureRandom
import java.util.*
import javax.crypto.Cipher
import javax.crypto.spec.SecretKeySpec
import javax.crypto.spec.IvParameterSpec

/**
 * Authentication and authorization system for Foundry IDE
 * Handles user authentication, plugin publishing, and marketplace access
 */
@Serializable
data class UserCredentials(
    val username: String,
    val email: String,
    val token: String,
    val refreshToken: String,
    val expiresAt: Long,
    val permissions: List<String>
)

@Serializable
data class AuthToken(
    val accessToken: String,
    val refreshToken: String,
    val tokenType: String = "Bearer",
    val expiresIn: Long,
    val scope: String
)

@Serializable
data class PluginSigningKey(
    val keyId: String,
    val publicKey: String,
    val privateKey: String, // Encrypted
    val algorithm: String = "RSA",
    val createdAt: Long,
    val expiresAt: Long? = null
)

@Serializable
data class PublisherProfile(
    val id: String,
    val name: String,
    val email: String,
    val website: String? = null,
    val description: String? = null,
    val verified: Boolean = false,
    val plugins: List<String> = emptyList(),
    val reputation: Double = 0.0
)

enum class AuthProvider {
    FOUNDRY, GITHUB, GOOGLE, MICROSOFT
}

enum class Permission {
    PLUGIN_PUBLISH,
    PLUGIN_DOWNLOAD,
    MARKETPLACE_ACCESS,
    BETA_ACCESS,
    ADMIN_ACCESS
}

class AuthenticationManager {
    private val json = Json {
        prettyPrint = true
        ignoreUnknownKeys = true
    }

    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private var currentUser: UserCredentials? = null
    private val authFile = getAuthFile()
    private val keysFile = getKeysFile()

    // Encryption key for sensitive data
    private val encryptionKey = generateEncryptionKey()

    init {
        loadStoredCredentials()
    }

    /**
     * Authenticate user with credentials
     */
    suspend fun authenticate(
        username: String,
        password: String,
        provider: AuthProvider = AuthProvider.FOUNDRY
    ): Result<UserCredentials> {
        return try {
            when (provider) {
                AuthProvider.FOUNDRY -> authenticateWithFoundry(username, password)
                AuthProvider.GITHUB -> authenticateWithGitHub(username, password)
                AuthProvider.GOOGLE -> authenticateWithGoogle(username, password)
                AuthProvider.MICROSOFT -> authenticateWithMicrosoft(username, password)
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    /**
     * Authenticate with OAuth provider
     */
    suspend fun authenticateWithOAuth(
        provider: AuthProvider,
        authorizationCode: String
    ): Result<UserCredentials> {
        return try {
            when (provider) {
                AuthProvider.GITHUB -> authenticateGitHubOAuth(authorizationCode)
                AuthProvider.GOOGLE -> authenticateGoogleOAuth(authorizationCode)
                AuthProvider.MICROSOFT -> authenticateMicrosoftOAuth(authorizationCode)
                else -> Result.failure(IllegalArgumentException("OAuth not supported for $provider"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    /**
     * Refresh authentication token
     */
    suspend fun refreshToken(): Result<UserCredentials> {
        val current = currentUser ?: return Result.failure(IllegalStateException("No user logged in"))

        return try {
            val response = makeAuthenticatedRequest(
                "POST",
                "/auth/refresh",
                mapOf("refreshToken" to current.refreshToken)
            )

            if (response.success) {
                val newCredentials = UserCredentials(
                    username = current.username,
                    email = current.email,
                    token = response.data["accessToken"] as String,
                    refreshToken = response.data["refreshToken"] as String,
                    expiresAt = System.currentTimeMillis() + (response.data["expiresIn"] as Long * 1000),
                    permissions = response.data["permissions"] as List<String>
                )

                currentUser = newCredentials
                saveCredentials(newCredentials)

                Result.success(newCredentials)
            } else {
                logout()
                Result.failure(Exception(response.error ?: "Token refresh failed"))
            }
        } catch (e: Exception) {
            logout()
            Result.failure(e)
        }
    }

    /**
     * Check if user is authenticated
     */
    fun isAuthenticated(): Boolean {
        val user = currentUser ?: return false

        // Check if token is expired
        if (System.currentTimeMillis() >= user.expiresAt) {
            scope.launch { refreshToken() }
            return false
        }

        return true
    }

    /**
     * Get current user
     */
    fun getCurrentUser(): UserCredentials? = currentUser

    /**
     * Check if user has permission
     */
    fun hasPermission(permission: Permission): Boolean {
        val user = currentUser ?: return false
        return permission.name.lowercase() in user.permissions ||
               "admin" in user.permissions ||
               "*" in user.permissions
    }

    /**
     * Logout user
     */
    fun logout() {
        currentUser = null
        authFile.delete()
        keysFile.delete()
    }

    /**
     * Generate plugin signing keys
     */
    fun generateSigningKeys(): PluginSigningKey {
        val keyPair = generateRSAKeyPair()
        val keyId = UUID.randomUUID().toString()

        val signingKey = PluginSigningKey(
            keyId = keyId,
            publicKey = encodePublicKey(keyPair.public),
            privateKey = encryptPrivateKey(encodePrivateKey(keyPair.private)),
            algorithm = "RSA",
            createdAt = System.currentTimeMillis(),
            expiresAt = null
        )

        saveSigningKeys(signingKey)
        return signingKey
    }

    /**
     * Sign plugin for publishing
     */
    fun signPlugin(pluginData: ByteArray, keyId: String): String {
        val keys = loadSigningKeys()
        val key = keys[keyId] ?: throw IllegalArgumentException("Signing key not found")

        val privateKey = decryptPrivateKey(key.privateKey)
        return signData(pluginData, privateKey)
    }

    /**
     * Verify plugin signature
     */
    fun verifyPluginSignature(pluginData: ByteArray, signature: String, publicKey: String): Boolean {
        return verifySignature(pluginData, signature, publicKey)
    }

    /**
     * Get publisher profile
     */
    suspend fun getPublisherProfile(publisherId: String): Result<PublisherProfile> {
        return try {
            val response = makeAuthenticatedRequest("GET", "/publishers/$publisherId")

            if (response.success) {
                val profile = PublisherProfile(
                    id = response.data["id"] as String,
                    name = response.data["name"] as String,
                    email = response.data["email"] as String,
                    website = response.data["website"] as? String,
                    description = response.data["description"] as? String,
                    verified = response.data["verified"] as Boolean,
                    plugins = response.data["plugins"] as List<String>,
                    reputation = response.data["reputation"] as Double
                )
                Result.success(profile)
            } else {
                Result.failure(Exception(response.error ?: "Failed to get publisher profile"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    /**
     * Update publisher profile
     */
    suspend fun updatePublisherProfile(profile: PublisherProfile): Result<Unit> {
        return try {
            val response = makeAuthenticatedRequest(
                "PUT",
                "/publishers/${profile.id}",
                mapOf(
                    "name" to profile.name,
                    "email" to profile.email,
                    "website" to profile.website,
                    "description" to profile.description
                )
            )

            if (response.success) {
                Result.success(Unit)
            } else {
                Result.failure(Exception(response.error ?: "Failed to update publisher profile"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    // Private implementation methods

    private suspend fun authenticateWithFoundry(username: String, password: String): Result<UserCredentials> {
        val response = makeRequest(
            "POST",
            "/auth/login",
            mapOf(
                "username" to username,
                "password" to hashPassword(password)
            )
        )

        return if (response.success) {
            val credentials = UserCredentials(
                username = username,
                email = response.data["email"] as String,
                token = response.data["accessToken"] as String,
                refreshToken = response.data["refreshToken"] as String,
                expiresAt = System.currentTimeMillis() + (response.data["expiresIn"] as Long * 1000),
                permissions = response.data["permissions"] as List<String>
            )

            currentUser = credentials
            saveCredentials(credentials)

            Result.success(credentials)
        } else {
            Result.failure(Exception(response.error ?: "Authentication failed"))
        }
    }

    private suspend fun authenticateWithGitHub(username: String, password: String): Result<UserCredentials> {
        // GitHub authentication implementation
        return Result.failure(NotImplementedError("GitHub authentication not implemented"))
    }

    private suspend fun authenticateWithGoogle(username: String, password: String): Result<UserCredentials> {
        // Google authentication implementation
        return Result.failure(NotImplementedError("Google authentication not implemented"))
    }

    private suspend fun authenticateWithMicrosoft(username: String, password: String): Result<UserCredentials> {
        // Microsoft authentication implementation
        return Result.failure(NotImplementedError("Microsoft authentication not implemented"))
    }

    private suspend fun authenticateGitHubOAuth(code: String): Result<UserCredentials> {
        val response = makeRequest(
            "POST",
            "/auth/github/oauth",
            mapOf("code" to code)
        )

        return parseOAuthResponse(response)
    }

    private suspend fun authenticateGoogleOAuth(code: String): Result<UserCredentials> {
        val response = makeRequest(
            "POST",
            "/auth/google/oauth",
            mapOf("code" to code)
        )

        return parseOAuthResponse(response)
    }

    private suspend fun authenticateMicrosoftOAuth(code: String): Result<UserCredentials> {
        val response = makeRequest(
            "POST",
            "/auth/microsoft/oauth",
            mapOf("code" to code)
        )

        return parseOAuthResponse(response)
    }

    private fun parseOAuthResponse(response: ApiResponse): Result<UserCredentials> {
        return if (response.success) {
            val credentials = UserCredentials(
                username = response.data["username"] as String,
                email = response.data["email"] as String,
                token = response.data["accessToken"] as String,
                refreshToken = response.data["refreshToken"] as String,
                expiresAt = System.currentTimeMillis() + (response.data["expiresIn"] as Long * 1000),
                permissions = response.data["permissions"] as List<String>
            )

            currentUser = credentials
            saveCredentials(credentials)

            Result.success(credentials)
        } else {
            Result.failure(Exception(response.error ?: "OAuth authentication failed"))
        }
    }

    private fun hashPassword(password: String): String {
        val digest = MessageDigest.getInstance("SHA-256")
        val hash = digest.digest(password.toByteArray())
        return hash.joinToString("") { "%02x".format(it) }
    }

    private fun generateEncryptionKey(): SecretKeySpec {
        val key = ByteArray(32)
        SecureRandom().nextBytes(key)
        return SecretKeySpec(key, "AES")
    }

    private fun encryptPrivateKey(privateKey: String): String {
        val cipher = Cipher.getInstance("AES/CBC/PKCS5Padding")
        val iv = ByteArray(16)
        SecureRandom().nextBytes(iv)
        val ivSpec = IvParameterSpec(iv)

        cipher.init(Cipher.ENCRYPT_MODE, encryptionKey, ivSpec)
        val encrypted = cipher.doFinal(privateKey.toByteArray())

        // Combine IV and encrypted data
        val combined = iv + encrypted
        return Base64.getEncoder().encodeToString(combined)
    }

    private fun decryptPrivateKey(encryptedKey: String): String {
        val combined = Base64.getDecoder().decode(encryptedKey)
        val iv = combined.copyOfRange(0, 16)
        val encrypted = combined.copyOfRange(16, combined.size)

        val cipher = Cipher.getInstance("AES/CBC/PKCS5Padding")
        val ivSpec = IvParameterSpec(iv)

        cipher.init(Cipher.DECRYPT_MODE, encryptionKey, ivSpec)
        val decrypted = cipher.doFinal(encrypted)

        return String(decrypted)
    }

    private fun generateRSAKeyPair(): java.security.KeyPair {
        val keyGen = java.security.KeyPairGenerator.getInstance("RSA")
        keyGen.initialize(2048)
        return keyGen.generateKeyPair()
    }

    private fun encodePublicKey(publicKey: java.security.PublicKey): String {
        return Base64.getEncoder().encodeToString(publicKey.encoded)
    }

    private fun encodePrivateKey(privateKey: java.security.PrivateKey): String {
        return Base64.getEncoder().encodeToString(privateKey.encoded)
    }

    private fun signData(data: ByteArray, privateKey: String): String {
        val key = java.security.KeyFactory.getInstance("RSA")
            .generatePrivate(java.security.spec.PKCS8EncodedKeySpec(Base64.getDecoder().decode(privateKey)))

        val signature = java.security.Signature.getInstance("SHA256withRSA")
        signature.initSign(key)
        signature.update(data)

        return Base64.getEncoder().encodeToString(signature.sign())
    }

    private fun verifySignature(data: ByteArray, signature: String, publicKey: String): Boolean {
        return try {
            val key = java.security.KeyFactory.getInstance("RSA")
                .generatePublic(java.security.spec.X509EncodedKeySpec(Base64.getDecoder().decode(publicKey)))

            val sig = java.security.Signature.getInstance("SHA256withRSA")
            sig.initVerify(key)
            sig.update(data)

            sig.verify(Base64.getDecoder().decode(signature))
        } catch (e: Exception) {
            false
        }
    }

    private fun saveCredentials(credentials: UserCredentials) {
        val encrypted = encryptData(json.encodeToString(UserCredentials.serializer(), credentials))
        authFile.parentFile?.mkdirs()
        authFile.writeText(encrypted)
    }

    private fun loadStoredCredentials() {
        try {
            if (authFile.exists()) {
                val encrypted = authFile.readText()
                val decrypted = decryptData(encrypted)
                currentUser = json.decodeFromString(UserCredentials.serializer(), decrypted)
            }
        } catch (e: Exception) {
            // If loading fails, clear stored credentials
            authFile.delete()
        }
    }

    private fun saveSigningKeys(key: PluginSigningKey) {
        val keys = loadSigningKeys().toMutableMap()
        keys[key.keyId] = key

        val encrypted = encryptData(json.encodeToString(
            (MapSerializer(String.serializer(), PluginSigningKey.serializer())),
            keys
        ))
        keysFile.parentFile?.mkdirs()
        keysFile.writeText(encrypted)
    }

    private fun loadSigningKeys(): Map<String, PluginSigningKey> {
        return try {
            if (keysFile.exists()) {
                val encrypted = keysFile.readText()
                val decrypted = decryptData(encrypted)
                json.decodeFromString(
                    (MapSerializer(String.serializer(), PluginSigningKey.serializer())),
                    decrypted
                )
            } else {
                emptyMap()
            }
        } catch (e: Exception) {
            emptyMap()
        }
    }

    private fun encryptData(data: String): String {
        val cipher = Cipher.getInstance("AES/CBC/PKCS5Padding")
        val iv = ByteArray(16)
        SecureRandom().nextBytes(iv)
        val ivSpec = IvParameterSpec(iv)

        cipher.init(Cipher.ENCRYPT_MODE, encryptionKey, ivSpec)
        val encrypted = cipher.doFinal(data.toByteArray())

        val combined = iv + encrypted
        return Base64.getEncoder().encodeToString(combined)
    }

    private fun decryptData(encryptedData: String): String {
        val combined = Base64.getDecoder().decode(encryptedData)
        val iv = combined.copyOfRange(0, 16)
        val encrypted = combined.copyOfRange(16, combined.size)

        val cipher = Cipher.getInstance("AES/CBC/PKCS5Padding")
        val ivSpec = IvParameterSpec(iv)

        cipher.init(Cipher.DECRYPT_MODE, encryptionKey, ivSpec)
        val decrypted = cipher.doFinal(encrypted)

        return String(decrypted)
    }

    private fun getAuthFile(): File {
        val userHome = System.getProperty("user.home")
        return File(userHome, ".foundry-ide/auth.json.enc")
    }

    private fun getKeysFile(): File {
        val userHome = System.getProperty("user.home")
        return File(userHome, ".foundry-ide/keys.json.enc")
    }

    // Network request helpers
    private data class ApiResponse(
        val success: Boolean,
        val data: Map<String, Any> = emptyMap(),
        val error: String? = null
    )

    private suspend fun makeRequest(method: String, endpoint: String, data: Map<String, Any> = emptyMap()): ApiResponse {
        // Implementation for making HTTP requests to the authentication server
        // This would use a proper HTTP client
        return ApiResponse(success = false, error = "Not implemented")
    }

    private suspend fun makeAuthenticatedRequest(method: String, endpoint: String, data: Map<String, Any> = emptyMap()): ApiResponse {
        val token = currentUser?.token ?: return ApiResponse(success = false, error = "Not authenticated")

        // Add authorization header
        val headers = mapOf("Authorization" to "Bearer $token")

        // Implementation for making authenticated HTTP requests
        return ApiResponse(success = false, error = "Not implemented")
    }
}

// Global authentication manager instance
val authManager = AuthenticationManager()