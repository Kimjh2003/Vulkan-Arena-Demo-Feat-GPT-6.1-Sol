package dev.demo.arena

import android.content.Context
import android.graphics.Typeface
import android.widget.LinearLayout
import android.widget.TextView
import org.json.JSONObject
import java.util.Locale

internal data class HudState @JvmOverloads constructor(
    val score: Int, val ammo: Int, val magazine: Int, val hp: Int,
    val seconds: Int, val wave: Int, val manual: Boolean,
    val reloading: Boolean, val ended: Boolean, val victory: Boolean,
    val weapon: Int = 0, val upgrading: Boolean = false, val dash: Float = 0f
) {
    companion object {
        fun parse(payload: String): HudState {
            val json = JSONObject(payload)
            return HudState(json.getInt("score"), json.getInt("ammo"), json.getInt("magazine"),
                json.getInt("hp"), json.getInt("seconds"), json.getInt("wave"),
                json.getBoolean("manual"), json.getBoolean("reloading"),
                json.getBoolean("ended"), json.getBoolean("victory"),json.optInt("weapon",0),
                json.optBoolean("upgrading",false),json.optDouble("dash",0.0).toFloat())
        }
    }
}

/** Small corner readout; no full-width status bar. */
internal class GameHud(context: Context) : LinearLayout(context) {
    private val primary = TextView(context)
    private val ammo = TextView(context)
    private val health = TextView(context)
    private val detail = TextView(context)
    private val message = TextView(context)
    private var lastState: HudState? = null
    init {
        orientation = VERTICAL
        background = ArenaTheme.card(context, 0xd92c2c2a.toInt())
        setPadding(dp(10), dp(6), dp(10), dp(6))
        fun line(view: TextView, size: Float, color: Int) {
            view.textSize = size; view.setTextColor(color)
            addView(view)
        }
        line(primary, 15f, ArenaTheme.text)
        primary.typeface = Typeface.create("sans-serif-medium", Typeface.NORMAL)
        val vitals = LinearLayout(context).apply { orientation = HORIZONTAL }
        for (view in listOf(ammo, health)) {
            view.textSize = 13f
            vitals.addView(view, LayoutParams(-2, -2).apply { marginEnd = dp(12) })
        }
        addView(vitals)
        line(detail, 11f, ArenaTheme.secondary)
        line(message, 11f, ArenaTheme.warning)
        message.visibility = GONE
        primary.text = "준비 중…"
    }
    private fun dp(value: Int) = ArenaTheme.dp(context, value)
    fun show(state: HudState) {
        if (state == lastState) return
        lastState = state
        val time = String.format(Locale.ROOT, "%02d:%02d", state.seconds / 60, state.seconds % 60)
        primary.text = "점수 ${state.score}  ·  $time"
        ammo.text = "탄약 ${state.ammo}/${state.magazine}"
        ammo.setTextColor(if (state.reloading || state.ammo <= 3) ArenaTheme.warning else ArenaTheme.info)
        health.text = "체력 ${state.hp}/5"
        health.setTextColor(if (state.hp <= 2) ArenaTheme.danger else ArenaTheme.success)
        detail.text = "${weaponNames[state.weapon.coerceIn(0,2)]} · 웨이브 ${state.wave}/5 · ${if (state.manual) "수동" else "자동"}"
        message.visibility = if (state.reloading && !state.ended && !state.victory) VISIBLE else GONE
        message.text = "재장전 중…"
        contentDescription = "${primary.text}, ${ammo.text}, ${health.text}, ${detail.text}"
    }
    fun showError(error: String) {
        message.visibility = VISIBLE
        message.text = "$error · 뒤로: 홈"
        message.setTextColor(ArenaTheme.danger)
    }
}
