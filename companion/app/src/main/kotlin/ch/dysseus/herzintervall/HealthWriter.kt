package ch.dysseus.herzintervall

import android.content.Context
import androidx.health.connect.client.HealthConnectClient
import androidx.health.connect.client.permission.HealthPermission
import androidx.health.connect.client.records.HeartRateVariabilityRmssdRecord
import androidx.health.connect.client.records.HydrationRecord
import androidx.health.connect.client.records.metadata.Device
import androidx.health.connect.client.records.metadata.Metadata
import androidx.health.connect.client.units.Volume
import java.time.Instant
import java.time.ZoneId

/**
 * Schreibt in Health Connect: HRV von Herzintervall, getrunkenes Wasser von
 * Drinktervall.
 *
 * Beide Datentypen gibt es dort fertig — HeartRateVariabilityRmssdRecord führt
 * RMSSD in Millisekunden, HydrationRecord ein Volumen. Es muss also nichts
 * umgerechnet oder in ein fremdes Feld gebogen werden.
 */
class HealthWriter(private val context: Context) {

    /** HRV-Messung eintragen. */
    suspend fun write(reading: Reading): String {
        val client = ready() ?: return unavailable()
        if (!granted(client, WRITE_HRV)) return PERMISSION_MISSING

        // Der Zeitpunkt ist der der MESSUNG, nicht der des Empfangs. Die Uhr
        // schickt ihn mit; läge stattdessen der Augenblick des Schreibens in
        // der Akte, wären die Werte um die Laufzeit der Übertragung verschoben.
        val at = Instant.ofEpochSecond(reading.epochSeconds)
        val record = HeartRateVariabilityRmssdRecord(
            time = at,
            zoneOffset = ZoneId.systemDefault().rules.getOffset(at),
            heartRateVariabilityMillis = reading.rmssdMs.toDouble(),
            metadata = watchMetadata(),
        )
        client.insertRecords(listOf(record))
        return "${reading.rmssdMs} ms eingetragen"
    }

    /** Ein getrunkenes Glas eintragen. */
    suspend fun writeWater(ml: Long, epochSeconds: Long): String {
        val client = ready() ?: return unavailable()
        if (!granted(client, WRITE_HYDRATION)) return PERMISSION_MISSING

        // HydrationRecord braucht eine Spanne, keinen Zeitpunkt. Ein Glas
        // trinkt sich nicht in null Sekunden; eine Minute ist eine ehrliche
        // Näherung und verhindert eine Spanne der Länge null, die Health
        // Connect ablehnt.
        val from = Instant.ofEpochSecond(epochSeconds)
        val to = from.plusSeconds(60)
        val zone = ZoneId.systemDefault().rules.getOffset(from)
        val record = HydrationRecord(
            startTime = from,
            startZoneOffset = zone,
            endTime = to,
            endZoneOffset = zone,
            volume = Volume.milliliters(ml.toDouble()),
            metadata = watchMetadata(),
        )
        client.insertRecords(listOf(record))
        return "$ml ml Wasser eingetragen"
    }

    // --- gemeinsam ---

    private fun ready(): HealthConnectClient? {
        return when (HealthConnectClient.getSdkStatus(context)) {
            HealthConnectClient.SDK_AVAILABLE -> HealthConnectClient.getOrCreate(context)
            else -> null
        }
    }

    private fun unavailable(): String {
        return when (HealthConnectClient.getSdkStatus(context)) {
            HealthConnectClient.SDK_UNAVAILABLE_PROVIDER_UPDATE_REQUIRED ->
                "Health Connect muss aktualisiert werden"
            else -> "Health Connect ist auf diesem Geraet nicht verfuegbar"
        }
    }

    private suspend fun granted(client: HealthConnectClient, permission: String): Boolean {
        return client.permissionController.getGrantedPermissions().contains(permission)
    }

    private fun watchMetadata(): Metadata = Metadata.autoRecorded(
        device = Device(
            manufacturer = "Core Devices",
            model = "Pebble Time 2",
            type = Device.TYPE_WATCH,
        )
    )

    companion object {
        const val PERMISSION_MISSING = "Erlaubnis fehlt - App oeffnen und erteilen"

        val WRITE_HRV: String =
            HealthPermission.getWritePermission(HeartRateVariabilityRmssdRecord::class)
        val WRITE_HYDRATION: String =
            HealthPermission.getWritePermission(HydrationRecord::class)

        // Beide zusammen werden angefragt: wer die App benutzt, will sie ganz.
        // Getrennt zu fragen hiesse zwei Dialoge fuer einen Zweck.
        val PERMISSIONS: Set<String> = setOf(WRITE_HRV, WRITE_HYDRATION)
    }
}
