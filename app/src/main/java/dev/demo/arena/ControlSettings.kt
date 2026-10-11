package dev.demo.arena

import android.content.Context

internal data class ControlSettings(
    val manualAim: Boolean = false,
    val leftHanded: Boolean = false,
    val sound: Boolean = true
)

internal class SettingsStore(context: Context) {
    private val prefs = context.getSharedPreferences("controls", Context.MODE_PRIVATE)
    fun load() = ControlSettings(
        prefs.getBoolean("manual_aim", false),
        prefs.getBoolean("left_handed", false),
        prefs.getBoolean("sound", true)
    )
    fun save(settings: ControlSettings) {
        prefs.edit().putBoolean("manual_aim", settings.manualAim)
            .putBoolean("left_handed", settings.leftHanded)
            .putBoolean("sound", settings.sound).apply()
    }
}
