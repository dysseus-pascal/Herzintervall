package ch.dysseus.herzintervall

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent

/**
 * Fährt den Empfangsdienst nach einem Neustart wieder hoch.
 *
 * BOOT_COMPLETED ist einer der wenigen impliziten Broadcasts, die ein im
 * Manifest angemeldeter Empfänger auch ab Android 8 noch bekommt — er steht auf
 * der Ausnahmeliste. Für `com.getpebble.action.app.RECEIVE` gilt das nicht,
 * genau deshalb gibt es den Dienst überhaupt.
 */
class BootReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        if (intent.action != Intent.ACTION_BOOT_COMPLETED) return
        ReceiverService.start(context)
    }
}
