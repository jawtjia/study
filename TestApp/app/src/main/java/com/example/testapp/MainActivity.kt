package com.example.testapp

import android.app.AlertDialog
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material.MaterialTheme
import androidx.compose.material.Surface
import androidx.compose.material.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import com.chaquo.python.Python
import com.chaquo.python.android.AndroidPlatform
import com.example.testapp.ui.theme.TestAppTheme


class MainActivity : ComponentActivity() {
    private external fun sumFromJNI(a: Int,b: Int): Int
    private external fun overloadPointComma()
    private external fun overloadPlusplus()
    private external fun constructor(): Int
    private external fun overloadTest(): String
    private external fun smartPointTest(): Int
    private external fun virtualTest(a: Int = 3,b: Int = 5): Int
    private external fun constTest(i: Int): Int
    companion object {
        init {
            System.loadLibrary("native-lib")
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        var c = sumFromJNI(1,2)
        overloadPointComma()
        overloadPlusplus()
        var d = constructor()
        var e = overloadTest()
        var f = smartPointTest()
        var v1 = virtualTest(2,4)
        var v2 = virtualTest()
        var ct = constTest(4)
        // 初始化Python环境
        if (!Python.isStarted()) {
            Python.start(AndroidPlatform(this))
        }
        val python = Python.getInstance() // 初始化Python环境
        val pyObject = python.getModule("pythontest") //"text"为需要调用的Python文件名
        val res = pyObject.callAttr("sayHello") //"sayHello"为需要调用的函数名

        super.onCreate(savedInstanceState)
        setContent {
            TestAppTheme {
                // A surface container using the 'background' color from the theme
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colors.background
                ) {
                    var show = "Android\n"
                    show += "$c $d $e $f\n"
                    show += "Rectangle1 Area is $v1\n"
                    show += "Rectangle2 Area is $v2\n"
                    show += "Const Test is $ct\n"
                    Greeting(show)
                }
            }
        }
        val textTips = AlertDialog.Builder(this@MainActivity)
            .setTitle("Tips:")
            .setMessage("" + res)
            .create()
        textTips.show()
    }
}

@Composable
fun Greeting(name: String) {
    Text(text = "Hello $name")
}

@Preview(showBackground = true)
@Composable
fun DefaultPreview() {
    TestAppTheme {
        Greeting("Android")
    }
}