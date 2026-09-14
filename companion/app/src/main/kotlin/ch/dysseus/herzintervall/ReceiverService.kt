package ch.dysseus.herzintervall

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Build
import android.os.IBinder
import android.util.Log

/**
 * Hält den Empfänger am Leben.
 *
 * WARUM DAS SEIN MUSS - und warum ein Eintrag im Manifest nicht reicht:
 * die Pebble-App verschickt eingehende AppMessages so:
 *
 *     val intent = Intent("com.getpebble.action.app.RECEIVE").apply { ... }
 *     context.sendOrderedBroadcast(intent, null)
 *
 * Ohne Paket und ohne Komponente - das ist ein IMPLIZITER Broadcast. Und seit
 * Android 8 bekommen im Manifest angemeldete Empfänger solche Broadcasts nicht
 * mehr; nur zur LAUFZEIT angemeldete bekommen sie noch. Der Empfänger stand
 * zuerst allein im Manifest und wurde deshalb nie aufgerufen, ganz gleich was
 * die Uhr schickte.
 *
 * Ein zur Laufzeit angemeldeter Empfänger braucht aber einen laufenden Prozess.
 * Deshalb dieser Vordergrunddienst mit seiner stillen Meldung: er ist der
 * Preis dafür, dass Messungen auch dann ankommen, wenn die App nicht offen ist.
 */
class ReceiverService : Service() {

    private var receiver: PebbleReceiver? = null

    override fun onCreate() {
        super.onCreate()
        startForeground(NOTIFICATION_ID, buildNotification())

        val r = PebbleReceiver()
        val filter = IntentFilter(PebbleReceiver.ACTION_RECEIVE)
        if (Build.VERSION.SDK_INT >= 33) {
            // Ab Android 13 muss ausdrücklich stehen, ob fremde Apps senden
            // dürfen. Sie dürfen - genau darum geht es hier.
            registerReceiver(r, filter, Context.RECEIVER_EXPORTED)
        } else {
            @Suppress("UnspecifiedRegisterReceiverFlag")
            registerReceiver(r, filter)
        }
        receiver = r
        Log.i(PebbleReceiver.TAG, "Empfaenger zur Laufzeit angemeldet")
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        // START_STICKY: wird der Dienst wegen Speichermangels beendet, startet
        // ihn das System wieder. Ohne das verpasst man die naechste Messung,
        // ohne es zu merken.
        return START_STICKY
    }

    override fun onDestroy() {
        receiver?.let {
            try {
                unregisterReceiver(it)
            } catch (e: IllegalArgumentException) {
                // war nicht angemeldet - nichts zu tun
            }
        }
        receiver = null
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    private fun buildNotification(): Notification {
        val mgr = getSystemService(NotificationManager::class.java)
        if (Build.VERSION.SDK_INT >= 26) {
            val channel = NotificationChannel(
                CHANNEL_ID,
                getString(R.string.channel_name),
                // Niedrigste Stufe: die Meldung muss da sein, damit der Dienst
                // laufen darf, soll aber nicht stoeren.
                NotificationManager.IMPORTANCE_MIN,
            )
            channel.setShowBadge(false)
            mgr.createNotificationChannel(channel)
        }
        val open = PendingIntent.getActivity(
            this, 0, Intent(this, MainActivity::class.java),
            PendingIntent.FLAG_IMMUTABLE,
        )
        return Notification.Builder(this, CHANNEL_ID)
            .setSmallIcon(android.R.drawable.ic_menu_myplaces)
            .setContentTitle(getString(R.string.app_name))
            .setContentText(getString(R.string.service_running))
            .setContentIntent(open)
            .setOngoing(true)
            .build()
    }

    companion object {
        private const val CHANNEL_ID = "empfang"
        private const val NOTIFICATION_ID = 1

        fun start(context: Context) {
            val intent = Intent(context, ReceiverService::class.java)
            if (Build.VERSION.SDK_INT >= 26) {
                context.startForegroundService(intent)
            } else {
                context.startService(intent)
            }
        }
    }
}
