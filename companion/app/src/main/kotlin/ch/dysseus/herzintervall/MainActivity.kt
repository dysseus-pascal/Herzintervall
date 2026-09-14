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
 * Bewusst ohne Compose und ohne Layout-Dateien - die App besteht im Kern aus
 * einem Empfaenger, und ein Bildschirm, den man dreimal im Leben oeffnet,
 * rechtfertigt keinen Baukasten. Weniger Abhaengigkeiten heisst hier auch:
 * weniger, was an einer ungetesteten App schiefgehen kann.
 */
class MainActivity : ComponentActivity() {

    private lateinit var status: TextView
    private lateinit var permissionLauncher: androidx.activity.result.ActivityResultLauncher<Set<String>>

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val contract: ActivityResultContract<Set<String>, Set<String>> =
            PermissionController.createRequestPermissionResultContract()
        permissionLauncher = registerForActivityResult(contract) { refresh() }

        // Ab Android 13 muss die Meldung des Vordergrunddienstes erlaubt sein.
        // Ohne sie laeuft der Dienst zwar, aber das System darf ihn frueher
        // beenden - und man sieht nicht, dass er ueberhaupt da ist.
        if (Build.VERSION.SDK_INT >= 33 &&
            checkSelfPermission(android.Manifest.permission.POST_NOTIFICATIONS)
                != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(arrayOf(android.Manifest.permission.POST_NOTIFICATIONS), 1)
        }

        // Der Empfaenger muss zur Laufzeit angemeldet sein, sonst erreicht
        // ihn der implizite Broadcast der Pebble-App nicht. Siehe
        // ReceiverService.
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
        lifecycleScope.launch {
            status.text = buildStatus()
        }
    }

    private suspend fun buildStatus(): String {
        val sb = StringBuilder()

        when (HealthConnectClient.getSdkStatus(this)) {
            HealthConnectClient.SDK_UNAVAILABLE -> {
                sb.append(getString(R.string.hc_missing))
                return sb.toString()
            }
            HealthConnectClient.SDK_UNAVAILABLE_PROVIDER_UPDATE_REQUIRED -> {
                sb.append(getString(R.string.hc_update))
                return sb.toString()
            }
        }

        val granted = try {
            HealthConnectClient.getOrCreate(this)
                .permissionController.getGrantedPermissions()
                .contains(HealthWriter.WRITE_PERMISSION)
        } catch (t: Throwable) {
            false
        }
        sb.append(getString(if (granted) R.string.perm_ok else R.string.perm_missing))
        sb.append("\n")
        sb.append(getString(R.string.service_hint))
        sb.append("\n\n")

        val last = Store(this).last()
        if (last == null) {
            sb.append(getString(R.string.nothing_yet))
        } else {
            val (r, result) = last
            val when_ = DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT)
                .format(Date(r.epochSeconds * 1000))
            sb.append(getString(R.string.last_reading, when_))
            sb.append("\n")
            sb.append("RMSSD ${r.rmssdMs} ms · ${r.bpm}/min · ${r.beats} ")
            sb.append(getString(R.string.beats))
            sb.append("\n")
            sb.append(getString(R.string.quality, r.dropped, r.coveredPercent))
            sb.append("\n\n")
            sb.append(result)
        }
        return sb.toString()
    }
}
