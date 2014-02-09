#ifndef console_receiver_impl_INCLUDED
#define console_receiver_impl_INCLUDED

class console_receiver_impl : public console_receiver
{
public:
	virtual void print(const char * p_message, unsigned p_message_length);
};

#endif // console_receiver_impl_INCLUDED
