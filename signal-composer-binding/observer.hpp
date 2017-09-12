#include <vector>

class Observer
{
public:
	virtual void update(double timestamp, double value) = 0;
};

class Subject
{
	double timestamp_;
	double value_;
	std::vector<Observer*> m_views;
public:
	void attach(Observer *obs)
	{
		m_views.push_back(obs);
	}
	void set_val(double timestamp, double value)
	{
		timestamp_ = timestamp;
		value_ = value;
		notify();
	}
	void notify()
	{
		for (int i = 0; i < m_views.size(); ++i)
		  m_views[i]->update(timestamp_, value_);
	}
};
