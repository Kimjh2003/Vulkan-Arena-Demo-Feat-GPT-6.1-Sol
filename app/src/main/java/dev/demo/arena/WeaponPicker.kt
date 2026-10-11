package dev.demo.arena

import android.content.Context

/** Presentation only; the caller persists the selected weapon. */
internal object WeaponPicker {
    private val descriptions=arrayOf("권총 · 12발 / 균형", "산탄총 · 6발 / 5발 확산", "기관단총 · 30발 / 빠른 연사")
    fun button(context: Context, selected: Int, onSelected: (Int)->Unit) =
        ArenaTheme.button(context,"무기: ${weaponNames[selected.coerceIn(0,2)]}") {
            show(context,selected,onSelected)
        }
    fun show(context: Context, selected: Int, onSelected: (Int)->Unit) {
        val dialog = ArenaDialog(context, "무기 선택", "선택한 무기로 다음 전투를 시작해")
        dialog.choices(descriptions.toList(),selected.coerceIn(0,2),onSelected)
        dialog.action("닫기") { dialog.dismiss() }
        dialog.show()
    }
}
