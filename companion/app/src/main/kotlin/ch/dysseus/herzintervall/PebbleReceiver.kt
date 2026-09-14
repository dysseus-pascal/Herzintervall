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

/** Welche Watchapp hat geschickt. */
enum class App { HERZINTERVALL, DRINKTERVALL }

/**
 * Nimmt entgegen, was die Uhren schicken: HRV-Messungen von Herzintervall und
 * getrunkene Gläser von Drinktervall.
 *
 * WIE DIE NACHRICHT HIERHER KOMMT: die Pebble-App sendet eingehende
 * AppMessages als gewöhnlichen Broadcast weiter. Das ist die klassische
 * PebbleKit-Schnittstelle, und sie braucht keine einzige Bibliothek — nur die
 * Intent-Namen unten. Nachzulesen in coredevices/mobileapp unter
 * libpebble3/.../pebblekit/classic/PebbleKitClassic.kt.
 *
 * DREI DINGE, DIE DABEI LEICHT SCHIEFGEHEN, alle hier einmal falsch gemacht:
 *
 *  1. Die Watchapp darf KEINEN companionApp-Eintrag in ihrer package.json
 *     haben. Ein Paketname dort wählt PebbleKit2, und das bindet sich an einen
 *     Dienst, statt zu senden (CompanionAppLifecycleManager.android.kt).
 *  2. Die UUID im Intent ist ein java.util.UUID, kein String.
 *  3. Der Broadcast ist implizit, erreicht also ab Android 8 keinen im
 *     Manifest angemeldeten Empfänger — siehe ReceiverService.
 *
 * Dass Drinktervall trotz eigener pkjs-Telefonseite hier ankommt, liegt an
 * appMessageToMultipleCompanions in LibPebbleConfig.kt: der Schalter steht
 * standardmässig auf true, AppMessages gehen dann an PKJS UND an die
 * klassische Companion-App.
 */
class PebbleReceiver : BroadcastReceiver() {

    override fun onReceive(context: Context, intent: Intent) {
        if (intent.action != ACTION_RECEIVE) return

        val from = whichApp(intent) ?: return   // gilt einer anderen Watchapp
        val transactionId = intent.getIntExtra(EXTRA_TRANSACTION_ID, -1)
        val data = intent.getStringExtra(EXTRA_MSG_DATA)
        if (data == null) {
            Log.w(TAG, "Nachricht ohne Daten")
            return
        }
        val values = parseDictionary(data)

        // Zuerst bestätigen, dann arbeiten. Die Uhr wartet auf das ACK; käme es
        // erst nach dem Schreiben, liefe sie in ihren Zeitablauf, sobald Health
        // Connect einmal langsam ist. Bestätigt wird JEDE Nachricht der beiden
        // Apps, auch eine ohne verwertbaren Inhalt — sonst meldete die Uhr
        // einen Fehler, wo keiner ist.
        ackTo(context, transactionId)

        when (from) {
            App.HERZINTERVALL -> handleHrv(context, values, data)
            App.DRINKTERVALL -> handleWater(context, values)
        }
    }

    private fun handleHrv(context: Context, values: Map<Int, Long>, raw: String) {
        val rmssd = values[KEY_RMSSD]
        if (rmssd == null || rmssd <= 0L) {
            Log.w(TAG, "Kein brauchbarer RMSSD in der Nachricht: $raw")
            return
        }
        val reading = Reading(
            rmssdMs = rmssd,
            bpm = values[KEY_BPM] ?: 0L,
            beats = values[KEY_BEATS] ?: 0L,
            dropped = values[KEY_DROPPED] ?: 0L,
            coveredPercent = values[KEY_COVERED] ?: 0L,
            epochSeconds = values[KEY_WHEN] ?: (System.currentTimeMillis() / 1000),
        )
        Log.i(TAG, "Messung erhalten: $reading")
        Store(context).rememberReading(reading)
        inBackground(context) { HealthWriter(context).write(reading) }
    }

    /**
     * Drinktervall meldet ein getrunkenes Glas.
     *
     * Die beiden Felder stehen NUR in der Nachricht, die unmittelbar auf einen
     * Trinkvorgang folgt — die übrigen Standmeldungen der App tragen sie nicht.
     * Ohne diese Unterscheidung trüge die Akte bei jedem Aufwachen der Uhr ein
     * weiteres Glas ein.
     */
    private fun handleWater(context: Context, values: Map<Int, Long>) {
        val at = values[KEY_DRANK_AT] ?: return
        val ml = values[KEY_GLASS_ML] ?: return
        if (at <= 0L || ml <= 0L) return

        // Doppelte abweisen: dieselbe Sekunde wird nur einmal eingetragen. Eine
        // erneut zugestellte Nachricht — etwa nach einem verlorenen ACK — darf
        // kein zweites Glas erzeugen.
        val store = Store(context)
        if (store.lastDrinkAt() == at) {
            Log.i(TAG, "Glas um $at schon eingetragen, uebersprungen")
            return
        }
        store.rememberDrink(at, ml)
        Log.i(TAG, "Glas erhalten: $ml ml um $at")
        inBackground(context) { HealthWriter(context).writeWater(ml, at) }
    }

    private fun inBackground(context: Context, block: suspend () -> String) {
        // goAsync hält den Prozess am Leben, bis das Schreiben fertig ist —
        // sonst darf das System uns mitten im Vorgang beenden.
        val pending = goAsync()
        CoroutineScope(Dispatchers.IO).launch {
            try {
                val result = block()
                Store(context).rememberResult(result)
                Log.i(TAG, "Health Connect: $result")
            } catch (t: Throwable) {
                Log.e(TAG, "Schreiben fehlgeschlagen", t)
                Store(context).rememberResult("Fehler: " + t.message)
            } finally {
                pending.finish()
            }
        }
    }

    /**
     * Von welcher Watchapp kommt die Nachricht?
     *
     * Die Pebble-App legt die UUID als java.util.UUID ins Intent, NICHT als
     * Zeichenkette: putExtra(APP_UUID, appMessageData.uuid.toJavaUuid()) in
     * PebbleKitClassic.kt. Ein getStringExtra() liefert dafür null — und der
     * Empfänger verwarf damit anfangs jede Nachricht in der ersten Zeile.
     * Beide Formen werden gelesen, weil die klassische PebbleKit-Doku von einer
     * Zeichenkette spricht.
     */
    private fun whichApp(intent: Intent): App? {
        val raw: Any? = if (android.os.Build.VERSION.SDK_INT >= 33) {
            intent.getSerializableExtra(EXTRA_UUID, UUID::class.java)
        } else {
            @Suppress("DEPRECATION")
            intent.getSerializableExtra(EXTRA_UUID)
        }
        val uuid: UUID? = when {
            raw is UUID -> raw
            else -> try {
                UUID.fromString(raw as? String ?: intent.getStringExtra(EXTRA_UUID))
            } catch (e: Exception) {
                null
            }
        }
        if (uuid == null) return null
        return when (uuid) {
            UUID.fromString(UUID_HERZINTERVALL) -> App.HERZINTERVALL
            UUID.fromString(UUID_DRINKTERVALL) -> App.DRINKTERVALL
            else -> null
        }
    }

    private fun ackTo(context: Context, transactionId: Int) {
        if (transactionId < 0) return
        // DIE RICHTIGE LEITUNG IST ...app.ACK, NICHT ...app.RECEIVE_ACK.
        // PebbleKitClassic.kt lauscht für die Bestätigung einer eingehenden
        // Nachricht auf INTENT_APP_ACK. RECEIVE_ACK ist die Gegenrichtung —
        // das schickt die Pebble-App hinaus, wenn die UHR etwas bestätigt hat.
        // Ein ACK dorthin hört niemand, und die Uhr lief in SEND_TIMEOUT.
        //
        // Kein setPackage: der Empfänger der Pebble-App ist zur Laufzeit
        // angemeldet und bekommt implizite Broadcasts.
        val ack = Intent(ACTION_ACK).apply {
            putExtra(EXTRA_TRANSACTION_ID, transactionId)
        }
        context.sendBroadcast(ack)
    }

    /**
     * Das klassische PebbleKit-Format: ein JSON-Feld je Eintrag mit key, type,
     * length und value. Uns interessieren nur ganze Zahlen — beide Watchapps
     * schicken ausschliesslich solche.
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
                    else -> Unit   // Zeichenketten und Rohdaten kommen nicht vor
                }
            }
        } catch (e: Exception) {
            Log.e(TAG, "Nachricht nicht lesbar: $json", e)
        }
        return out
    }

    companion object {
        const val TAG = "Herzintervall"

        // Aus den package.json der beiden Watchapps. Ändert sich eine UUID
        // dort, muss sie hier mit — sonst verwirft der Empfänger jede Nachricht
        // der betreffenden App.
        const val UUID_HERZINTERVALL = "7e1b28b2-cd13-4b50-ac4e-187b92707707"
        const val UUID_DRINKTERVALL = "5b0f7a3e-2c8d-4b61-9e4f-7d2a1c9b8e50"

        // Nummern der Schlüssel, von waf aus package.json vergeben (appKeys in
        // appinfo.json im fertigen .pbw). Die REIHENFOLGE der messageKeys-Liste
        // bestimmt sie — eine neue Zeile DAZWISCHEN verschiebt alle folgenden.
        const val KEY_RMSSD = 10000
        const val KEY_BPM = 10001
        const val KEY_BEATS = 10002
        const val KEY_DROPPED = 10003
        const val KEY_COVERED = 10004
        const val KEY_WHEN = 10005

        // Drinktervall zählt eigenständig: REQUEST, GLASSES, NEXT_TIME,
        // NEXT_INDEX, COUNT, SLOTS, LANG, TARGET, GLASS_ML, DRANK_AT.
        const val KEY_GLASS_ML = 10008
        const val KEY_DRANK_AT = 10009

        const val ACTION_RECEIVE = "com.getpebble.action.app.RECEIVE"
        // Bestätigung einer Nachricht, die von der UHR kam.
        const val ACTION_ACK = "com.getpebble.action.app.ACK"
        const val EXTRA_UUID = "uuid"
        const val EXTRA_TRANSACTION_ID = "transaction_id"
        const val EXTRA_MSG_DATA = "msg_data"
        // Nur als Hinweis, wer der Gegenpart ist — gesetzt wird er nirgends.
        const val CORE_APP_PACKAGE = "coredevices.coreapp"
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
