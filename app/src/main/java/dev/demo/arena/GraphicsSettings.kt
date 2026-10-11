package dev.demo.arena
import android.content.Context

/** Codec preference is independent of control settings and saved immediately. */
internal class GraphicsSettings(context: Context) {
    private val prefs=context.getSharedPreferences("graphics",Context.MODE_PRIVATE)
    var textureFamily: Int
        get()=prefs.getInt("texture_family",0).coerceIn(0,3)
        set(value) {prefs.edit().putInt("texture_family",value.coerceIn(0,3)).apply()}
}
internal object GraphicsPicker {
    fun show(context: Context, store: GraphicsSettings) {
        val dialog=ArenaDialog(context,"그래픽 설정","다음 전투에 적용 · 미지원 형식은 자동 대체")
        dialog.choices(listOf("자동 · 지원되는 형식 선택","ETC1 · RGB","ETC2 · RGB","ASTC · 4×4"),store.textureFamily) {
            store.textureFamily=it
        }
        dialog.action("닫기") {dialog.dismiss()}
        dialog.show()
    }
}
