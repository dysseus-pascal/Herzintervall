package ch.dysseus.herzintervall

import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.view.Gravity
import android.view.ViewGroup
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContract
import androidx.health.connect.client.HealthConnectClient
import androidx.health.connect.client.PermissionController
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.launch
import java.text.DateFormat
import java.util.Date

/**
 * Die einzige Ansicht: Zustand zeigen und die Erlaubnis erfragen.
 *
 * Bewusst ohne Compose und ohne Layout-Dateien — die App besteht im Kern aus
 * einem Empfänger, und ein Bildschirm, den man dreimal im Leben öffnet,
 * rechtfertigt keinen Baukasten. Weniger Abhängigkeiten heisst hier auch:
 * weniger, was schiefgehen kann an einer App, die sich nur am Telefon selbst
 * erproben lässt.
 */
class MainActivity : ComponentActivity() {

    private lateinit var status: TextView
    private lateinit var permissionLauncher:
        androidx.activity.result.ActivityResultLauncher<Set<String>>

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val contract: ActivityResultContract<Set<String>, Set<String>> =
            PermissionController.createRequestPermissionResultContract()
        permissionLauncher = registerForActivityResult(contract) { refresh() }

        // Ab Android 13 muss die Meldung des Vordergrunddienstes erlaubt sein.
        // Ohne sie läuft der Dienst zwar, aber das System darf ihn früher
        // beenden — und man sieht nicht, dass er überhaupt da ist.
        if (Build.VERSION.SDK_INT >= 33 &&
            checkSelfPermission(android.Manifest.permission.POST_NOTIFICATIONS)
                != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(arrayOf(android.Manifest.permission.POST_NOTIFICATIONS), 1)
        }

        // Der Empfänger muss zur Laufzeit angemeldet sein, sonst erreicht ihn
        // der implizite Broadcast der Pebble-App nicht. Siehe ReceiverService.
        ReceiverService.start(this)

        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(48, 64, 48, 48)
        }

        root.addView(TextView(this).apply {
            text = getString(R.string.app_name)
            textSize = 26f
        })
        root.addView(TextView(this).apply {
            text = getString(R.string.intro)
            textSize = 15f
            setPadding(0, 24, 0, 32)
        })

        status = TextView(this).apply { textSize = 15f }
        root.addView(status)

        root.addView(Button(this).apply {
            text = getString(R.string.grant)
            setOnClickListener { permissionLauncher.launch(HealthWriter.PERMISSIONS) }
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = 40; gravity = Gravity.START }
        })

        setContentView(ScrollView(this).apply { addView(root) })
    }

    override fun onResume() {
        super.onResume()
        refresh()
    }

    private fun refresh() {
        lifecycleScope.launch { status.text = buildStatus() }
    }

    private suspend fun buildStatus(): String {
        val sb = StringBuilder()

        when (HealthConnectClient.getSdkStatus(this)) {
            HealthConnectClient.SDK_UNAVAILABLE ->
                return getString(R.string.hc_missing)
            HealthConnectClient.SDK_UNAVAILABLE_PROVIDER_UPDATE_REQUIRED ->
                return getString(R.string.hc_update)
        }

        val granted = try {
            HealthConnectClient.getOrCreate(this)
                .permissionController.getGrantedPermissions()
        } catch (t: Throwable) {
            emptySet<String>()
        }
        sb.append(line(granted.contains(HealthWriter.WRITE_HRV), R.string.perm_hrv))
        sb.append("\n")
        sb.append(line(granted.contains(HealthWriter.WRITE_HYDRATION), R.string.perm_water))
        sb.append("\n")
        sb.append(getString(R.string.service_hint))
        sb.append("\n\n")

        val store = Store(this)

        val reading = store.lastReading()
        if (reading == null) {
            sb.append(getString(R.string.nothing_yet))
        } else {
            sb.append(getString(R.string.last_reading, stamp(reading.epochSeconds)))
            sb.append("\n")
            sb.append("RMSSD ${reading.rmssdMs} ms · ${reading.bpm}/min · ${reading.beats} ")
            sb.append(getString(R.string.beats))
            sb.append("\n")
            sb.append(getString(R.string.quality, reading.dropped, reading.coveredPercent))
        }

        val drinkAt = store.lastDrinkAt()
        if (drinkAt > 0L) {
            sb.append("\n\n")
            sb.append(getString(R.string.last_drink, stamp(drinkAt), store.lastDrinkMl()))
        }

        val result = store.lastResult()
        if (result.isNotEmpty()) {
            sb.append("\n\n")
            sb.append(result)
        }
        return sb.toString()
    }

    private fun line(ok: Boolean, textId: Int): String =
        (if (ok) "✓ " else "✗ ") + getString(textId)

    private fun stamp(epochSeconds: Long): String =
        DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT)
            .format(Date(epochSeconds * 1000))
}
