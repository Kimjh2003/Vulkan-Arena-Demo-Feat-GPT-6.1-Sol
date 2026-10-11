package dev.demo.arena

import android.app.Dialog
import android.content.Context
import android.content.res.ColorStateList
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.view.Gravity
import android.view.Window
import android.widget.CompoundButton
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import android.widget.RadioButton
import android.widget.RadioGroup
import android.view.View

/** Shared modal layout: fixed actions, scrollable content, and Arena theme tokens. */
internal class ArenaDialog(context: Context, title: String, description: String) : Dialog(context) {
    val content = LinearLayout(context).apply { orientation = LinearLayout.VERTICAL }
    private val actions = LinearLayout(context).apply { gravity = Gravity.END }
    init {
        requestWindowFeature(Window.FEATURE_NO_TITLE)
        val root = LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            background = ArenaTheme.card(context)
            setPadding(dp(24), dp(8), dp(24), dp(8))
        }
        root.addView(TextView(context).apply {
            text = title; textSize = 22f; setTextColor(ArenaTheme.text)
            setTypeface(typeface, android.graphics.Typeface.BOLD)
        })
        root.addView(TextView(context).apply {
            text = description; textSize = 12f; setTextColor(ArenaTheme.secondary)
            setPadding(0, dp(2), 0, dp(6))
        })
        root.addView(ScrollView(context).apply { addView(content); isFillViewport = false },
            LinearLayout.LayoutParams(-1, 0, 1f))
        root.addView(actions, LinearLayout.LayoutParams(-1, -2).apply { topMargin = dp(6) })
        setContentView(root)
        setCanceledOnTouchOutside(true)
    }
    fun action(label: String, callback: () -> Unit) {
        actions.addView(ArenaTheme.button(context, label, callback),
            LinearLayout.LayoutParams(-2, dp(48)).apply { marginStart = dp(8) })
    }
    fun choices(labels: List<String>, selected: Int, choose: (Int) -> Unit) {
        val group=RadioGroup(context)
        labels.forEachIndexed { index,label ->
            val button=RadioButton(context).apply {
                id=View.generateViewId();text=label;isChecked=index==selected
            }
            group.addView(option(button))
            button.setOnClickListener { dismiss();choose(index) }
        }
        content.addView(group)
    }
    fun option(button: CompoundButton): CompoundButton = button.apply {
        textSize = 14f; setTextColor(ArenaTheme.text)
        buttonTintList = ColorStateList(arrayOf(intArrayOf(android.R.attr.state_checked), intArrayOf()),
            intArrayOf(ArenaTheme.info, ArenaTheme.secondary))
        setPadding(dp(12), dp(4), dp(12), dp(4))
        minHeight = dp(48)
        fun refresh() { background = ArenaTheme.card(context, if (isChecked) 0xff253545.toInt() else ArenaTheme.background) }
        refresh()
        setOnCheckedChangeListener { _, _ -> refresh() }
        layoutParams = LinearLayout.LayoutParams(-1, -2).apply { bottomMargin = dp(2) }
    }
    override fun show() {
        super.show()
        val metrics = context.resources.displayMetrics
        window?.apply {
            setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            setDimAmount(.65f)
            setLayout(minOf(dp(560), (metrics.widthPixels * .9f).toInt()),
                minOf(dp(420), (metrics.heightPixels * .9f).toInt()))
            setGravity(Gravity.CENTER)
        }
    }
    private fun dp(value: Int) = ArenaTheme.dp(context, value)
}
