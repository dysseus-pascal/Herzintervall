package ch.dysseus.herzintervall

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.util.Log
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import org.json.JSONArray
import java.util.UUID

/**
 * Nimmt die Messergebnisse der Uhr entgegen.
 *
 * WIE DIE NACHRICHT HIERHER KOMMT: die Pebble-App sendet eingehende
 * AppMessages als gewoehnlichen Broadcast weiter. Das ist die klassische
 * PebbleKit-Schnittstelle, und sie braucht keine einzige Bibliothek - nur die
 * drei Intent-Namen unten. Nachzulesen in coredevices/mobileapp unter
 * libpebble3/.../pebblekit/classic/PebbleKitClassic.kt.
 *
 * Damit die Pebble-App weiss, wohin: die Watchapp traegt diese App in ihrer
 * package.json unter companionApp.android.apps ein.
 *
 * WICHTIG: eine Watchapp kann NICHT beides haben. "PebbleKit JS cannot be used
 * in conjunction with PebbleKit Android or PebbleKit iOS" - liegt ein
 * src/pkjs/index.js im Projekt, gehen die Nachrichten dorthin und kommen hier
 * nie an. Herzintervall hat deshalb bewusst keinen JavaScript-Teil.
 */
class PebbleReceiver : BroadcastReceiver() {

    override fun onReceive(context: Context, intent: Intent) {
        if (intent.action != ACTION_RECEIVE) return

        val uuid = intent.getStringExtra(EXTRA_UUID)
        if (!sameUuid(uuid)) {
            // Der Broadcast gilt einer anderen Watchapp.
            return
        }
        val transactionId = intent.getIntExtra(EXTRA_TRANSACTION_ID, -1)
        val data = intent.getStringExtra(EXTRA_MSG_DATA)
        if (data == null) {
            Log.w(TAG, "Nachricht ohne Daten")
            return
        }

        val values = parseDictionary(data)
        val rmssd = values[KEY_RMSSD]
        if (rmssd == null || rmssd <= 0L) {
            Log.w(TAG, "Kein brauchbarer RMSSD in der Nachricht: $data")
            return
        }

        // Zuerst bestaetigen, dann arbeiten. Die Uhr wartet auf das ACK; wuerde
        // es erst nach dem Schreiben kommen, liefe sie in ihren Zeitablauf,
        // sobald Health Connect einmal langsam ist.
        ackTo(context, transactionId)

        val reading = Reading(
            rmssdMs = rmssd,
            bpm = values[KEY_BPM] ?: 0L,
            beats = values[KEY_BEATS] ?: 0L,
            dropped = values[KEY_DROPPED] ?: 0L,
            coveredPercent = values[KEY_COVERED] ?: 0L,
            epochSeconds = values[KEY_WHEN] ?: (System.currentTimeMillis() / 1000),
        )
        Log.i(TAG, "Messung erhalten: $reading")

        // goAsync haelt den Prozess am Leben, bis das Schreiben fertig ist -
        // sonst darf das System uns mitten im Vorgang beenden.
        val pending = goAsync()
        CoroutineScope(Dispatchers.IO).launch {
            try {
                val result = HealthWriter(context).write(reading)
                Store(context).remember(reading, result)
                Log.i(TAG, "Health Connect: $result")
            } catch (t: Throwable) {
                Log.e(TAG, "Schreiben fehlgeschlagen", t)
                Store(context).remember(reading, "Fehler: ${t.message}")
            } finally {
                pending.finish()
            }
        }
    }

    private fun sameUuid(raw: String?): Boolean {
        if (raw == null) return false
        return try {
            UUID.fromString(raw) == UUID.fromString(WATCHAPP_UUID)
        } catch (e: IllegalArgumentException) {
            false
        }
    }

    private fun ackTo(context: Context, transactionId: Int) {
        if (transactionId < 0) return
        val ack = Intent(ACTION_RECEIVE_ACK).apply {
            putExtra(EXTRA_TRANSACTION_ID, transactionId)
            setPackage(PEBBLE_PACKAGE)
        }
        context.sendBroadcast(ack)
    }

    /**
     * Das klassische PebbleKit-Format: ein JSON-Feld je Eintrag mit key, type,
     * length und value. Uns interessieren nur ganze Zahlen - die Watchapp
     * schickt ausschliesslich solche.
     */
    private fun parseDictionary(json: String): Map<Int, Long> {
        val out = HashMap<Int, Long>()
        try {
            val arr = JSONArray(json)
            for (i in 0 until arr.length()) {
                val item = arr.optJSONObject(i) ?: continue
                val key = item.optInt("key", -1)
                if (key < 0) continue
                when (item.optString("type", "int")) {
                    "int", "uint" -> out[key] = item.optLong("value")
                    else -> Unit   // Zeichenketten und Rohdaten kommen hier nicht vor
                }
            }
        } catch (e: Exception) {
            Log.e(TAG, "Nachricht nicht lesbar: $json", e)
        }
        return out
    }

    companion object {
        const val TAG = "Herzintervall"

        // Aus der package.json der Watchapp. Aendert sich die UUID dort, muss
        // sie hier mit - sonst verwirft der Empfaenger jede Nachricht.
        const val WATCHAPP_UUID = "7e1b28b2-cd13-4b50-ac4e-187b92707707"

        // Nummern der Schluessel, von waf aus package.json vergeben (appKeys in
        // appinfo.json im fertigen .pbw). Die Reihenfolge der messageKeys-Liste
        // bestimmt sie - eine neue Zeile DAZWISCHEN verschiebt alle folgenden.
        const val KEY_RMSSD = 10000
        const val KEY_BPM = 10001
        const val KEY_BEATS = 10002
        const val KEY_DROPPED = 10003
        const val KEY_COVERED = 10004
        const val KEY_WHEN = 10005

        const val ACTION_RECEIVE = "com.getpebble.action.app.RECEIVE"
        const val ACTION_RECEIVE_ACK = "com.getpebble.action.app.RECEIVE_ACK"
        const val EXTRA_UUID = "uuid"
        const val EXTRA_TRANSACTION_ID = "transaction_id"
        const val EXTRA_MSG_DATA = "msg_data"
        const val PEBBLE_PACKAGE = "com.getpebble.android.basalt"
    }
}

data class Reading(
    val rmssdMs: Long,
    val bpm: Long,
    val beats: Long,
    val dropped: Long,
    val coveredPercent: Long,
    val epochSeconds: Long,
)
