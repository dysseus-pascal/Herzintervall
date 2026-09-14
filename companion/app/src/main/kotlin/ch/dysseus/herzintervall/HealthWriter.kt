package ch.dysseus.herzintervall

import android.content.Context
import androidx.health.connect.client.HealthConnectClient
import androidx.health.connect.client.permission.HealthPermission
import androidx.health.connect.client.records.HeartRateVariabilityRmssdRecord
import androidx.health.connect.client.records.metadata.Device
import androidx.health.connect.client.records.metadata.Metadata
import java.time.Instant
import java.time.ZoneId

/**
 * Schreibt eine Messung in Health Connect.
 *
 * HeartRateVariabilityRmssdRecord passt genau: Health Connect fuehrt RMSSD in
 * Millisekunden als eigenen Datentyp. Es muss also nichts umgerechnet oder in
 * ein fremdes Feld gebogen werden - die Uhr misst dasselbe, was die Akte
 * erwartet.
 */
class HealthWriter(private val context: Context) {

    suspend fun write(reading: Reading): String {
        when (HealthConnectClient.getSdkStatus(context)) {
            HealthConnectClient.SDK_UNAVAILABLE ->
                return "Health Connect ist auf diesem Geraet nicht verfuegbar"
            HealthConnectClient.SDK_UNAVAILABLE_PROVIDER_UPDATE_REQUIRED ->
                return "Health Connect muss aktualisiert werden"
        }

        val client = HealthConnectClient.getOrCreate(context)
        val granted = client.permissionController.getGrantedPermissions()
        if (!granted.contains(WRITE_PERMISSION)) {
            // Ohne Erlaubnis ist das kein Fehler, sondern ein offener Schritt:
            // die App einmal oeffnen und die Erlaubnis erteilen.
            return "Erlaubnis fehlt - App oeffnen und erteilen"
        }

        // Der Zeitpunkt ist der der MESSUNG, nicht der des Empfangs. Die Uhr
        // schickt ihn mit; laege stattdessen der Augenblick des Schreibens in
        // der Akte, waeren die Werte um die Laufzeit der Uebertragung verschoben.
        val at = Instant.ofEpochSecond(reading.epochSeconds)
        val record = HeartRateVariabilityRmssdRecord(
            time = at,
            zoneOffset = ZoneId.systemDefault().rules.getOffset(at),
            heartRateVariabilityMillis = reading.rmssdMs.toDouble(),
            metadata = Metadata.autoRecorded(
                device = Device(
                    manufacturer = "Core Devices",
                    model = "Pebble Time 2",
                    type = Device.TYPE_WATCH,
                )
            ),
        )
        client.insertRecords(listOf(record))
        return "${reading.rmssdMs} ms eingetragen"
    }

    companion object {
        val WRITE_PERMISSION: String =
            HealthPermission.getWritePermission(HeartRateVariabilityRmssdRecord::class)
        val PERMISSIONS: Set<String> = setOf(WRITE_PERMISSION)
    }
}
