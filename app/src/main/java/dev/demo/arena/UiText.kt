package dev.demo.arena

internal object UiText {
    fun clock(seconds: Int): String {
        val value=seconds.coerceAtLeast(0)
        val minutes=value/60;val remainder=value%60
        return "${if(minutes<10) "0" else ""}$minutes:${if(remainder<10) "0" else ""}$remainder"
    }
    fun dashTenths(cooldown: Float): Int = Math.round(cooldown.coerceAtLeast(0f)*10f)
    fun dash(tenths: Int): String = "대시 ${tenths/10}.${tenths%10}"
}
