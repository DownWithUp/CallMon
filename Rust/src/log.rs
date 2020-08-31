pub use winapi::km::wdm::DbgPrint;

#[macro_export]
macro_rules! log {
    ($string: expr) => {
        unsafe {
            $crate::log::DbgPrint(concat!("[>] ", $string, "\0").as_ptr())
        }
    };

    ($string: expr, $($x:tt)*) => {
        unsafe {
            #[allow(unused_unsafe)]
            $crate::log::DbgPrint(concat!("[>] ", $string, "\0").as_ptr(), $($x)*)
        }
    };
}
