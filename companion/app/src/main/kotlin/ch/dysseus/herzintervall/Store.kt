package ch.dysseus.herzintervall

import android.content.Context

/**
 * Merkt sich, was zuletzt ankam und was damit geschah.
 *
 * Nicht als Verlauf gedacht — der gehört in Health Connect, und dort steht er
 * auch. Das hier ist eine Statusanzeige: ohne sie sieht man einer App, die im
 * Kern aus einem Empfänger besteht, von aussen nie an, ob sie überhaupt etwas
 * tut.
 *
 * Eine Aufgabe hat der Speicher darüber hinaus: `lastDrinkAt` weist doppelte
 * Trinkmeldungen ab. Eine erneut zugestellte Nachricht — etwa nach einem
 * verlorenen ACK — darf kein zweites Glas in die Akte schreiben.
 */
class Store(context: Context) {
    private val prefs = context.getSharedPreferences("herzintervall", Context.MODE_PRIVATE)

    // --- HRV ---

    fun rememberReading(reading: Reading) {
        prefs.edit()
            .putLong("rmssd", reading.rmssdMs)
            .putLong("bpm", reading.bpm)
            .putLong("beats", reading.beats)
            .putLong("dropped", reading.dropped)
            .putLong("covered", reading.coveredPercent)
            .putLong("when", reading.epochSeconds)
            .apply()
    }

    fun lastReading(): Reading? {
        val rmssd = prefs.getLong("rmssd", 0L)
        if (rmssd <= 0L) return null
        return Reading(
            rmssdMs = rmssd,
            bpm = prefs.getLong("bpm", 0L),
            beats = prefs.getLong("beats", 0L),
            dropped = prefs.getLong("dropped", 0L),
            coveredPercent = prefs.getLong("covered", 0L),
            epochSeconds = prefs.getLong("when", 0L),
        )
    }

    // --- Wasser ---

    fun rememberDrink(epochSeconds: Long, ml: Long) {
        prefs.edit()
            .putLong("drink_at", epochSeconds)
            .putLong("drink_ml", ml)
            .apply()
    }

    fun lastDrinkAt(): Long = prefs.getLong("drink_at", 0L)
    fun lastDrinkMl(): Long = prefs.getLong("drink_ml", 0L)

    // --- Ergebnis des letzten Schreibversuchs, für beide Arten ---

    fun rememberResult(result: String) {
        prefs.edit()
            .putString("result", result)
            .putLong("result_at", System.currentTimeMillis() / 1000)
            .apply()
    }

    fun lastResult(): String = prefs.getString("result", "") ?: ""
}
