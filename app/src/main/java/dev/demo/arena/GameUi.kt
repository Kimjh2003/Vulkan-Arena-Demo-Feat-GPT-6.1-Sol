package dev.demo.arena

import android.content.Context
import android.view.Gravity
import android.widget.CheckBox
import android.widget.LinearLayout
import android.widget.RadioButton
import android.widget.RadioGroup
import android.widget.TextView

internal object GameUi {
    fun menu(context: Context, settings: ControlSettings, onPlay: () -> Unit,
             onSettings: () -> Unit) = LinearLayout(context).apply {
        orientation = LinearLayout.VERTICAL; gravity = Gravity.CENTER
        setBackgroundColor(ArenaTheme.background)
        addView(TextView(context).apply {
            text = "3D ARENA\n엄폐물을 활용해 5분 생존\n" +
                if (settings.manualAim) "양손 · 이동 + 직접 조준/사격" else "한 손 · 이동 + 자동 조준/사격"
            setTextColor(ArenaTheme.text); textSize = 21f; gravity = Gravity.CENTER
            setPadding(16, 12, 16, 12)
        })
        fun action(label: String, callback: () -> Unit) = ArenaTheme.button(context,label,callback)
        for (button in listOf(action("플레이", onPlay), action("조작 설정", onSettings))) {
            addView(button, LinearLayout.LayoutParams(ArenaTheme.dp(context, 280), ArenaTheme.dp(context, 48)).apply {
                topMargin = ArenaTheme.dp(context, 8)
            })
        }
        addView(TextView(context).apply {
            text = "뒤로가기를 누르면 종료"; textSize = 12f
            setTextColor(ArenaTheme.secondary); gravity = Gravity.CENTER
            setPadding(0, ArenaTheme.dp(context, 12), 0, 0)
        })
    }
    fun settings(context: Context, value: ControlSettings, onGraphics: () -> Unit = {}, onSave: (ControlSettings) -> Unit) {
        val dialog = ArenaDialog(context, "조작 설정", "조준 방식과 이동 스틱 위치를 선택해줘")
        val modes = RadioGroup(context)
        val auto = RadioButton(context).apply { id = android.view.View.generateViewId(); text = "한 손 · 자동 조준/사격" }
        val manual = RadioButton(context).apply { id = android.view.View.generateViewId(); text = "양손 · 직접 조준/사격" }
        modes.addView(dialog.option(auto)); modes.addView(dialog.option(manual))
        modes.check(if (value.manualAim) manual.id else auto.id)
        val left = CheckBox(context).apply { text = "이동 스틱을 오른쪽에 배치"; isChecked = value.leftHanded }
        val sound = CheckBox(context).apply { text = "효과음"; isChecked = value.sound }
        dialog.content.addView(modes)
        dialog.content.addView(dialog.option(left)); dialog.content.addView(dialog.option(sound))
        dialog.action("그래픽") { onGraphics() }
        dialog.action("취소") { dialog.dismiss() }
        dialog.action("저장") {
            dialog.dismiss()
            onSave(ControlSettings(modes.checkedRadioButtonId == manual.id, left.isChecked, sound.isChecked))
        }
        dialog.show()
    }
}
