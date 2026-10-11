package dev.demo.arena

import android.content.Context
import android.content.res.ColorStateList
import android.graphics.drawable.RippleDrawable
import android.widget.Button
import android.graphics.drawable.GradientDrawable

/** Adapted dark theme tokens from CopilotKit/OpenIntelligentUI (MIT).
 * See assets/licenses/OpenIntelligentUI-MIT.txt and THIRD_PARTY_NOTICES.md. */
internal object ArenaTheme {
    val background = 0xff1a1a18.toInt()
    val panel = 0xff2c2c2a.toInt()
    val text = 0xffe8e6de.toInt()
    val secondary = 0xff9c9a92.toInt()
    val info = 0xff85b7eb.toInt()
    val danger = 0xfff09595.toInt()
    val success = 0xff97c459.toInt()
    val warning = 0xffef9f27.toInt()
    fun button(context: Context, label: String, action: () -> Unit) = Button(context).apply {
        text=label;isAllCaps=false;textSize=14f;minimumHeight=dp(context,48)
        setPadding(dp(context,16),dp(context,4),dp(context,16),dp(context,4))
        setTextColor(ColorStateList(arrayOf(intArrayOf(-android.R.attr.state_enabled),intArrayOf()),
            intArrayOf(secondary,info)))
        background=RippleDrawable(ColorStateList.valueOf(0x3385b7eb),card(context),null)
        setOnClickListener {action()}
    }
    fun dp(context: Context, value: Int) = (value * context.resources.displayMetrics.density).toInt()
    fun card(context: Context, color: Int = panel) = GradientDrawable().apply {
        setColor(color)
        cornerRadius = dp(context, 12).toFloat()
        setStroke(dp(context, 1).coerceAtLeast(1), 0x26ffffff)
    }
}
