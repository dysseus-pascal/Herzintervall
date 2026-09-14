package ch.dysseus.herzintervall

import android.content.Context

/**
 * Merkt sich, was zuletzt ankam und was damit geschah.
 *
 * Nicht als Verlauf gedacht - der gehoert in Health Connect, und dort steht er
 * auch. Das hier ist eine Statusanzeige: ohne sie sieht man einer App, die nur
 * aus einem Empfaenger besteht, von aussen nie an, ob sie ueberhaupt etwas tut.
 */
class Store(context: Context) {
    private val prefs = context.getSharedPreferences("herzintervall", Context.MODE_PRIVATE)

    fun remember(reading: Reading, result: String) {
        prefs.edit()
            .putLong("rmssd", reading.rmssdMs)
            .putLong("bpm", reading.bpm)
            .putLong("beats", reading.beats)
            .putLong("dropped", reading.dropped)
            .putLong("covered", reading.coveredPercent)
            .putLong("when", reading.epochSeconds)
            .putString("result", result)
            .putLong("received", System.currentTimeMillis() / 1000)
            .apply()
    }

    fun last(): Pair<Reading, String>? {
        val rmssd = prefs.getLong("rmssd", 0L)
        if (rmssd <= 0L) return null
        val reading = Reading(
            rmssdMs = rmssd,
            bpm = prefs.getLong("bpm", 0L),
            beats = prefs.getLong("beats", 0L),
            dropped = prefs.getLong("dropped", 0L),
            coveredPercent = prefs.getLong("covered", 0L),
            epochSeconds = prefs.getLong("when", 0L),
        )
        return reading to (prefs.getString("result", "") ?: "")
    }
}
