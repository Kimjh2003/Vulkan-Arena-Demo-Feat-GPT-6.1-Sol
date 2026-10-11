package dev.demo.arena
import android.content.Context
internal val weaponNames = listOf("권총", "산탄총", "기관단총")
internal class ProgressStore(context: Context) {
    private val prefs=context.getSharedPreferences("progress",Context.MODE_PRIVATE)
    var weapon: Int
        get()=prefs.getInt("weapon",0).coerceIn(0,2)
        set(value) {prefs.edit().putInt("weapon",value.coerceIn(0,2)).apply()}
    fun record(state: HudState) {
        prefs.edit().putInt("score",maxOf(prefs.getInt("score",0),state.score))
            .putInt("seconds",maxOf(prefs.getInt("seconds",0),state.seconds))
            .putInt("wins",prefs.getInt("wins",0)+if(state.victory) 1 else 0).apply()
    }
    fun summary(): String {
        val seconds=prefs.getInt("seconds",0)
        return "최고 점수 ${prefs.getInt("score",0)} · 최장 생존 %02d:%02d · 성공 ${prefs.getInt("wins",0)}회".format(seconds/60,seconds%60)
    }
}
